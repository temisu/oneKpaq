/* Copyright (C) Teemu Suutari */

#include <map>
#include <tuple>
#include <string>
#include <memory>

#include "onekpaq_common.h"

#include "StreamCodec.hpp"
#include "ArithEncoder.hpp"
#include "ArithDecoder.hpp"
#include "BlockCodec.hpp"
#include "CacheFile.hpp"
#include "SimpleDispatch.hpp"
#include "Timer.hpp"

std::vector<u8> StreamCodec::CreateSingleStream()
{
	std::vector<u8> ret{'O','N','E','K','P','A','Q'};
	ret.push_back('4'+static_cast<uint>(_mode));
	ret.push_back(_shift);
	ret.push_back(0);
	for (auto &it:_header)
		ret.insert(ret.end(),it.begin(),it.end());

	if (_mode==EncodeMode::Multi||_mode==EncodeMode::MultiFast) {
		ret.push_back(0);
		ret.push_back(0);
	}
	ret.insert(ret.end(),_dest.begin(),_dest.end());
	return ret;
}

std::vector<u8> StreamCodec::CreateAsmHeader()
{
	// After a few optimization rounds this is messy and needs refactoring
	std::vector<u8> ret;
	for (auto hIt=_header.begin();hIt<_header.end();hIt++) {
		if (_mode==EncodeMode::Multi||_mode==EncodeMode::MultiFast) {
			uint length=(*hIt)[0]|uint((*hIt)[1])<<8;
			if (hIt==_header.begin()) length++;
			ret.push_back(length>>8);
			ret.push_back(length);
		}
		for (auto mIt=hIt->begin()+3;mIt<hIt->end();mIt++) {
			if (mIt+1==hIt->end()) ret.push_back(0);
				else ret.push_back(~*mIt);
		}
	}
	if (_mode==EncodeMode::Multi||_mode==EncodeMode::MultiFast) {
		ret.push_back(0);
		ret.push_back(0);
	}
	std::reverse(ret.begin(),ret.end());
#ifdef DEBUG_BUILD
	for (auto i:ret) DEBUG("%02x",i);
#endif
	return ret;
}

static BlockCodec::BlockCodecType StreamModetoBlockCodecType(StreamCodec::EncodeMode em)
{
	return (BlockCodec::BlockCodecType[]){
		BlockCodec::BlockCodecType::Single,
		BlockCodec::BlockCodecType::Standard,
		BlockCodec::BlockCodecType::Single,
		BlockCodec::BlockCodecType::Standard
	}[static_cast<uint>(em)-1];
}

void StreamCodec::Encode(const std::vector<std::vector<u8>> &blocks,EncodeMode mode,EncoderComplexity complexity,const std::string &cacheFileName)
{
	ASSERT(blocks.size(),"Empty blocks");
	ASSERT(uint(mode)&&mode<=EncodeMode::ModeLast,"Unknown mode");
	_mode=mode;
	ASSERT(uint(mode)&&complexity<=EncoderComplexity::ComplexityLast,"Unknown complexity mode");

	BlockCodec::BlockCodecType bct=StreamModetoBlockCodecType(_mode);

	// TODO: fix copy mess
	std::vector<std::vector<u8>> encodeBlocks;
	if (_mode!=EncodeMode::Multi&&_mode!=EncodeMode::MultiFast) {
		encodeBlocks.resize(1);
		for (auto &it:blocks) encodeBlocks[0].insert(encodeBlocks[0].end(),it.begin(),it.end());
	} else encodeBlocks=blocks;

	auto timeTaken=Timer([&]() {
		INFO("Starting onekpaq compression with %u blocks...",int(encodeBlocks.size()));

		typedef std::tuple<std::unique_ptr<BlockCodec>,uint,std::vector<u8>> BlockDef;
		std::vector<BlockDef> finalBlocks;

		CacheFile cf;
		if (!cacheFileName.empty() && cf.readFile(cacheFileName) && cf.getNumBlocks()==encodeBlocks.size()) {
			INFO("Using cache to skip brute force search");

			_shift=cf.getShift();
			for (uint cfi=0,i=0;i<encodeBlocks.size();i+=cf.getCombineData()[cfi++]) {
				std::vector<u8> blockData;
				for (uint j=0;j<cf.getCombineData()[cfi];j++)
					blockData.insert(blockData.end(),encodeBlocks[i+j].begin(),encodeBlocks[i+j].end());

				finalBlocks.push_back(std::make_tuple(std::unique_ptr<BlockCodec>(new BlockCodec(bct,_shift)),cf.getCombineData()[cfi],blockData));
				std::get<0>(finalBlocks.back())->SetHeader(cf.getHeader()[cfi],uint(blockData.size()));
			}
		} else {
			INFO("Cache does not exist or is not relevant. Starting brute force.");

			// TODO: needs refactoring. This is too regular structure to created this way.
			// (also parallelization was removed here, can be simplified)
			// in order not to encode multiple times same blocks we need a map
			typedef std::pair<uint,uint> BlockKeyDef;
			typedef std::vector<BlockKeyDef> CombinationDef;

			float currentBestLength=std::numeric_limits<float>::infinity();
			for (uint shift=1;shift<=16;shift++) {
				// create possible combinations
				std::map<BlockKeyDef,BlockDef> blockMap;
				std::vector<CombinationDef> combinations;
				if (encodeBlocks.size()==1) {
					BlockKeyDef key=std::make_pair(0,encodeBlocks[0].size());
					blockMap.insert(std::make_pair(key,std::make_tuple(std::unique_ptr<BlockCodec>(new BlockCodec(bct,shift)),1,encodeBlocks[0])));
					combinations.push_back(CombinationDef(1,key));
				} else {
					std::vector<u8> combinedBlocks;
					for (auto &it:encodeBlocks) combinedBlocks.insert(combinedBlocks.end(),it.begin(),it.end());
					uint combinationsCount=1<<(encodeBlocks.size()-1);
					combinations.resize(combinationsCount);
					for (uint i=0;i<combinationsCount;i++) {
						uint blockStart=0;
						uint blockLength=uint(encodeBlocks[0].size());
						uint blockCount=1;
						for (uint j=0;j<encodeBlocks.size()-1;j++) {
							if (i&(1<<j)) {
								// split
								BlockKeyDef key=std::make_pair(blockStart,blockLength);
								combinations[i].push_back(key);
								if (blockMap.find(key)==blockMap.end()) {
									blockMap.insert(std::make_pair(key,std::make_tuple(std::unique_ptr<BlockCodec>(new BlockCodec(bct,shift)),blockCount,std::vector<u8>(combinedBlocks.begin()+blockStart,combinedBlocks.begin()+blockStart+blockLength))));
								}
								blockStart+=blockLength;
								blockLength=0;
								blockCount=0;
							}
							// next block
							blockLength+=encodeBlocks[j+1].size();
							blockCount++;
						}
						// last block
						BlockKeyDef key=std::make_pair(blockStart,blockLength);
						combinations[i].push_back(key);
						if (blockMap.find(key)==blockMap.end()) {
							blockMap.insert(std::make_pair(key,std::make_tuple(std::unique_ptr<BlockCodec>(new BlockCodec(bct,shift)),blockCount,std::vector<u8>(combinedBlocks.begin()+blockStart,combinedBlocks.begin()+blockStart+blockLength))));
						}
					}
				}

				// process all possible combinations from map.
				bool multiRetry=complexity>=EncoderComplexity::High;
				bool multiInit=complexity>=EncoderComplexity::Medium;
				for (auto &it:blockMap) {
					std::get<0>(it.second)->CreateContextModels(std::get<2>(it.second),multiRetry,multiInit);
				}

				// find best
				auto &best=*std::min_element(combinations.begin(),combinations.end(),[&](const CombinationDef &a,const CombinationDef &b) {
					float aLength=0;
					for (auto &it:a) aLength+=std::get<0>(blockMap[it])->GetEstimatedLength();
					float bLength=0;
					for (auto &it:b) bLength+=std::get<0>(blockMap[it])->GetEstimatedLength();
					return aLength<bLength;
				});

				float tmpLength=0;
				for (auto &it:best) tmpLength+=std::get<0>(blockMap[it])->GetEstimatedLength();
				if (tmpLength<currentBestLength)
				{
					currentBestLength=tmpLength;
					_shift=shift;
					finalBlocks.clear();
					for (auto &it:best)
						finalBlocks.push_back(std::move(blockMap[it]));
				}
			}

			if (!cacheFileName.empty()) {
				cf.clear(uint(encodeBlocks.size()));
				cf.setShift(_shift);

				for (auto &it:finalBlocks) {
					cf.getCombineData().push_back(std::get<1>(it));
					cf.getHeader().push_back(std::get<0>(it)->GetHeader());
				}

				cf.writeFile(cacheFileName);
			}
		}

		const ArithEncoder::EncoderType encodeModeTranslation[]={
			ArithEncoder::EncoderType::Standard,
			ArithEncoder::EncoderType::SingleAsm,
			ArithEncoder::EncoderType::MultiAsm,
			ArithEncoder::EncoderType::SingleAsm,
			ArithEncoder::EncoderType::MultiAsm};

		ArithEncoder ecClean(ArithEncoder::EncoderType::Standard);
		ArithEncoder ecAsm(encodeModeTranslation[int(_mode)]);
		_header.clear();

		for (auto &it:finalBlocks) {
			std::get<0>(it)->Encode(std::get<2>(it),ecClean);
			std::get<0>(it)->Encode(std::get<2>(it),ecAsm);
			_header.push_back(std::get<0>(it)->GetHeader());
		}
		ecClean.Finalize();
		ecAsm.Finalize();
		_dest=ecClean.GetDest();
		auto tmp1=CreateAsmHeader();
		auto tmp2=ecAsm.GetDest();
		if (tmp1.size()<4)
		{
			// should not happen in practice. For now lets pad with 1-3 bytes
			_destAsm1.clear();
			_destAsm2.clear();
			for (uint i=tmp1.size();i<4;i++) _destAsm2.push_back(0);
			_destAsm2.insert(_destAsm2.end(),tmp1.begin(),tmp1.end());
			_destAsm2.insert(_destAsm2.end(),tmp2.begin(),tmp2.end());
		} else {
			_destAsm1.clear();
			_destAsm1.insert(_destAsm1.end(),tmp1.begin(),tmp1.end()-4);
			_destAsm2.clear();
			_destAsm2.insert(_destAsm2.end(),tmp1.end()-4,tmp1.end());
			_destAsm2.insert(_destAsm2.end(),tmp2.begin(),tmp2.end());
		}
		LAST_TICK();

		INFO("Compression done. Following blocks were created:");
		uint i=0,pos=0,totalHLength=0,totalLength=0;
		for (auto &it:finalBlocks) {
			uint len=std::get<0>(it)->GetRawLength();
			totalLength+=len;
			uint hLen=uint(std::get<0>(it)->GetHeader().size());
			totalHLength+=hLen;
			if (std::get<0>(it)->GetEstimatedLength()) {
				uint cLen=std::get<0>(it)->GetEstimatedLength()-hLen*8;
				float ratio=float(cLen+hLen*8)/float(len*8)*100.0f;
				INFO("Block%u 0x%08x-0x%08x, (%u+%.1f/%u) bytes, compression ratio %.2f%%, contexts: %s",i++,pos,pos+len-1,
					hLen,float(cLen*.125),len,ratio,std::get<0>(it)->PrintModels().c_str());
			} else {
				INFO("Block%u 0x%08x-0x%08x, (%u) bytes, contexts: %s",i++,pos,pos+len-1,
					len,std::get<0>(it)->PrintModels().c_str());
			}
			pos+=len;
		}
		if (i>1) totalHLength+=2; // multi block encoder needs a final marker
		float ratio=float(totalHLength+_dest.size())/float(totalLength)*100.0f;
		INFO("Total (%u+%u/%u), compression ratio %.2f%%",totalHLength,_dest.size(),totalLength,ratio);
		INFO("Asm stream total (%u+%u)",_destAsm1.size(),_destAsm2.size());
	});
	INFO("Encoding stream took %f seconds",float(timeTaken));
}

void StreamCodec::LoadStream(std::vector<u8> singleStream)
{
	// not safe in case "evil header" is constructed. Only some basis checks provided
	ASSERT(singleStream.size()>12,"Too short file");
	ASSERT(!memcmp(singleStream.data(),"ONEKPAQ",7),"oneKpaq header missing");
	_mode=static_cast<EncodeMode>(singleStream[7]-'4');
	ASSERT(uint(_mode)&&_mode<=EncodeMode::ModeLast,"Unknown mode");
	_shift=singleStream[8];
	ASSERT(_shift&&_shift<=16,"Wrong shift");
	ASSERT(!singleStream[9],"Unknown format");

	_header.clear();
	uint prevEnd=10;
	bool isSingle=_mode!=EncodeMode::Multi&&_mode!=EncodeMode::MultiFast;
	while (singleStream[prevEnd]||singleStream[prevEnd+1]) {
		uint headerEnd=singleStream[prevEnd+2]+prevEnd;
		_header.push_back(std::vector<u8>(singleStream.begin()+prevEnd,singleStream.begin()+headerEnd));
		prevEnd=headerEnd;
		if (isSingle) break;
	}
	_dest=std::vector<u8>(singleStream.begin()+prevEnd+(isSingle?0:2),singleStream.end());
}


std::vector<u8> StreamCodec::Decode()
{
	// now it is simple
	std::vector<u8> ret;
	auto timeTaken=Timer([&]() {
		ArithDecoder dc(_dest,ArithDecoder::DecoderType::Standard);
		for (auto &it:_header) {
			BlockCodec bc(StreamModetoBlockCodecType(_mode),_shift);
			auto block=bc.Decode(it,dc);
			ret.insert(ret.end(),block.begin(),block.end());
		}
	});
	INFO("Decoding stream took %f seconds",float(timeTaken));
	return ret;
}

std::vector<u8> StreamCodec::DecodeAsmStream()
{
	// This method is only for testing!
	const ArithDecoder::DecoderType decodeModeTranslation[]={
		ArithDecoder::DecoderType::Standard,
		ArithDecoder::DecoderType::SingleAsm,
		ArithDecoder::DecoderType::MultiAsm,
		ArithDecoder::DecoderType::SingleAsm,
		ArithDecoder::DecoderType::MultiAsm};

	std::vector<u8> ret;
	std::vector<u8> src;
	src.insert(src.end(),_destAsm2.begin()+4,_destAsm2.end());
	auto timeTaken=Timer([&]() {
		ArithDecoder dc(src,decodeModeTranslation[int(_mode)]);
		for (auto &it:_header) {
			BlockCodec bc(StreamModetoBlockCodecType(_mode),_shift);
			auto block=bc.Decode(it,dc);
			ret.insert(ret.end(),block.begin(),block.end());
		}
	});
	INFO("Decoding stream took %f seconds",float(timeTaken));
	return ret;
}
