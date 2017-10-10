/* Copyright (C) Teemu Suutari */

#include <algorithm>
#include <math.h>
#include <limits>
#include <random>

#include "onekpaq_common.h"

#include "BlockCodec.hpp"
#include "SimpleDispatch.hpp"

// intentionally not typedeffed float to disable math (needs mixer to do it properly)
struct BlockCodec::Probability
{
	float			p;
};

struct BlockCodec::BitCounts
{
	u32			c0;
	u32			c1;
};

static inline bool BitAtPos(const std::vector<u8> &src,uint bitPos)
{
	return src[bitPos>>3]&0x80U>>(bitPos&7);
}

class LogisticMixer
{
public:
	LogisticMixer() { }
	~LogisticMixer() = default;

	static BlockCodec::Probability Convert(const BlockCodec::BitCounts &counts)
	{
		// squashed probability
		return BlockCodec::Probability{log2f((float)counts.c1/(float)counts.c0)};
	}

	void Add(BlockCodec::Probability p,uint weight)
	{
		_p+=p.p/weight;
	}

	float GetLength() const
	{
		// stretch + probability to length
		return -log2f(1/(exp2f(_p)+1));
	}

private:
	float 			_p=0;
};

class PAQ1CountBooster
{
public:
	PAQ1CountBooster() = delete;
	~PAQ1CountBooster() = delete;

	static void Initialize(BlockCodec::BitCounts &counts)
	{
		counts.c0=0;
		counts.c1=0;
	}

	static void Increment(BlockCodec::BitCounts &counts,bool bitIsSet)
	{
		counts.c0++;
		counts.c1++;
		if (bitIsSet) counts.c0>>=1;
			else counts.c1>>=1;
	}

	static void Swap(BlockCodec::BitCounts &counts)
	{
		std::swap(counts.c0,counts.c1);
	}

	static void Finalize(BlockCodec::BitCounts &counts,uint shift)
	{
		counts.c0=(counts.c0<<shift)+1;
		counts.c1=(counts.c1<<shift)+1;
	}
};

class QWContextModel
{
public:
	class iterator
	{
		friend class QWContextModel;
	public:
		~iterator() = default;

		iterator& operator++()
		{
			if (_currentPos!=_parent._bitPos+8>>3) {
				_currentPos++;
				findNext();
			}
			return *this;
		}

		bool operator!=(const iterator &it)
		{
			return it._currentPos!=_currentPos;
		}

		std::pair<uint,bool> operator*()
		{
			// returns negative model mask + bit
			return _maskBit;
		}

	private:
		iterator(QWContextModel &parent,uint currentPos) :
			_parent(parent),
			_currentPos(currentPos)
		{

		}
		
		iterator& findNext()
		{
			const std::vector<u8> &data=_parent._data;
			uint bitPos=_parent._bitPos;
			uint bytePos=bitPos>>3;
			uint maxPos=bitPos+8>>3;
			bool overRunBit=bytePos==_parent._data.size();
			u8 mask=0xff00U>>(bitPos&7);

			while (_currentPos<maxPos) {
				if (_currentPos>=8&&_currentPos<bytePos&&(overRunBit||!((data[_currentPos]^data[bytePos])&mask))) {
					uint noMatchMask=0;
					for (uint i=1;i<9;i++)
						if (data[_currentPos-i]!=data[bytePos-i]) noMatchMask|=0x80>>i-1;

					_maskBit=std::make_pair(noMatchMask,BitAtPos(data,(_currentPos<<3)+(bitPos&7)));
					break;
				}
				_currentPos++;
			}
			return *this;
		}

		QWContextModel&	_parent;
		uint		_currentPos;
		std::pair<uint,bool> _maskBit;
	};

	QWContextModel(const std::vector<u8> &data,uint bitPos) :
		_data(data),
		_bitPos(bitPos)
	{

	}

	~QWContextModel() = default;

	static uint GetModelCount()
	{
		return 0xffU;
	}

	static uint GetStartOffset()
	{
		return 9;
	}

	iterator begin()
	{
		return iterator(*this,0).findNext();
	}

	iterator end()
	{
		return iterator(*this,_bitPos+8>>3);
	}

private:
	const std::vector<u8>&	_data;
	uint			_bitPos;
};

// TODO: too much like previous, combine
class NoLimitQWContextModel
{
public:
	class iterator
	{
		friend class NoLimitQWContextModel;
	public:
		~iterator() = default;

		iterator& operator++()
		{
			if (_currentPos!=_parent._bitPos+8>>3) {
				_currentPos++;
				findNext();
			}
			return *this;
		}

		bool operator!=(const iterator &it)
		{
			return it._currentPos!=_currentPos;
		}

		std::pair<uint,bool> operator*()
		{
			// returns negative model mask + bit
			return _maskBit;
		}

	private:
		iterator(NoLimitQWContextModel &parent,uint currentPos) :
			_parent(parent),
			_currentPos(currentPos)
		{

		}
		
		iterator& findNext()
		{
			const std::vector<u8> &data=_parent._data;
			uint bitPos=_parent._bitPos;
			uint bytePos=bitPos>>3;
			uint maxPos=bitPos+8>>3;
			bool overRunBit=bytePos==_parent._data.size();
			u8 mask=0xff00U>>(bitPos&7);

			while (_currentPos<maxPos) {
				if (bytePos<9&&!_currentPos) {
					u8 xShift=8-(bitPos&7);
					auto byteLookup=[&](uint pos)->u8 {
						if (pos>bytePos||bytePos==_parent._data.size()) return 0;
							else if (pos==bytePos) return data[pos]>>xShift;
								else return data[pos];
					};
					auto rol=[&](u8 byte,u8 count)->u8 {
						count&=7;
						return (byte>>8-count)|(byte<<count);
					};
					u8 xByte=byteLookup(8)^rol(byteLookup(bytePos),xShift);
					if (!(xByte>>xShift)) {
						uint noMatchMask=0;
						for (uint i=1;i<9;i++)
							if (byteLookup(8-i)!=byteLookup(bytePos-i)) noMatchMask|=0x80>>i-1;
						_maskBit=std::make_pair(noMatchMask,xByte>>xShift-1&1);
						break;
					}
				} else {
					if (_currentPos>=8&&_currentPos<bytePos&&(overRunBit||!((data[_currentPos]^data[bytePos])&mask))) {
						uint noMatchMask=0;
						for (uint i=1;i<9;i++)
							if (data[_currentPos-i]!=data[bytePos-i]) noMatchMask|=0x80>>i-1;

						_maskBit=std::make_pair(noMatchMask,BitAtPos(data,(_currentPos<<3)+(bitPos&7)));
						break;
					}

				}
				_currentPos++;
			}
			return *this;
		}

		NoLimitQWContextModel&	_parent;
		uint			_currentPos;
		std::pair<uint,bool>	_maskBit;
	};

	NoLimitQWContextModel(const std::vector<u8> &data,uint bitPos) :
		_data(data),
		_bitPos(bitPos)
	{

	}

	~NoLimitQWContextModel() = default;

	static uint GetModelCount()
	{
		return 0xffU;
	}

	static uint GetStartOffset()
	{
		return 0;
	}

	iterator begin()
	{
		return iterator(*this,0).findNext();
	}

	iterator end()
	{
		return iterator(*this,_bitPos+8>>3);
	}

private:
	const std::vector<u8>&	_data;
	uint			_bitPos;
};

template<class CM,class CB,class MIX>
class BlockCodec::CompressorModel : public BlockCodec::CompressorModelBase
{
public:
	CompressorModel() { }

	~CompressorModel() = default;

	virtual uint GetModelCount() const override
	{
		return CM::GetModelCount();
	}

	virtual uint GetStartOffset() const override
	{
		return CM::GetStartOffset();
	}

	virtual void CalculateAllProbabilities(std::vector<Probability> &dest,const std::vector<u8> &src,uint bitPos,uint shift) override
	{
		CM cm(src,bitPos);

		BitCounts counts[CM::GetModelCount()];
		for (auto &it:counts) CB::Initialize(it);
		for (auto m:cm) {
			uint i=0;
			for (auto &it:counts) {
				if (!(m.first&i)) CB::Increment(it,m.second);
				i++;
			}
		}
		dest.clear();
		for (auto &it:counts) {
			if (BitAtPos(src,bitPos)) CB::Swap(it);		// simplifies processing, Probabilities will be invariant from src
			CB::Finalize(it,shift);
			dest.push_back(MIX::Convert(it));
		}
	}

	virtual float EstimateBitLength(const std::vector<Probability> &probabilities,const std::vector<BlockCodec::Model> &models) override
	{
		MIX mixer;

		for (auto &it:models)
			if (it.weight) mixer.Add(probabilities[it.model],it.weight);

		return mixer.GetLength();
	}
	
	virtual u32 CalculateSubRange(const std::vector<u8> &src,int bitPos,const std::vector<BlockCodec::Model> &models,u32 range,uint shift) override;

	virtual void CreateWeightProfile(std::vector<std::pair<BitCounts,uint>> &dest,const std::vector<u8> &src,uint bitPos,const std::vector<BlockCodec::Model> &models,uint shift) override
	{
		CM cm(src,bitPos);

		dest.clear();
		BitCounts counts[models.size()];
		for (auto &it:counts) CB::Initialize(it);
		for (auto m:cm) {
			uint i=0;
			for (auto &it:models) {
				if (!(m.first&it.model)) CB::Increment(counts[i],m.second);
				i++;
			}
		}
		uint i=0;
		for (auto &it:models) {
			CB::Finalize(counts[i],shift);
			dest.push_back(std::make_pair(counts[i++],it.weight));
		}
	}
};

template<>
u32 BlockCodec::CompressorModel<QWContextModel,PAQ1CountBooster,LogisticMixer>::CalculateSubRange(const std::vector<u8> &src,int bitPos,const std::vector<BlockCodec::Model> &models,u32 range,uint shift)
{
	// implementation is correct. however, there might be rounding errors
	// ruining the result. It depends on the compilers and builtins whether
	// sqrtl reduces to fqsrt and lrintl reduces to fist.
	// please note the funny looking type-conversions here. they are for a reason
	ASSERT(bitPos>=-1,"Invalid pos");

	std::vector<std::pair<BitCounts,uint>> def;
	if (bitPos==-1) {
		// for the pre-bit no models says a thing
		BitCounts bc;
		PAQ1CountBooster::Initialize(bc);
		PAQ1CountBooster::Finalize(bc,shift);
		for (auto &it:models)
			def.push_back(std::make_pair(bc,it.weight));
	} else {
		CreateWeightProfile(def,src,bitPos,models,shift);
	}

	long double p=1;
	uint weight=def[0].second;
	for (auto &it:def) {
		while (weight!=it.second) {
			p=sqrtl(p);
			weight>>=1;
		}
		p=(long double)(int)it.first.c0/p;
		p=(long double)(int)it.first.c1/p;
	}
	while (weight!=1) {
		p=sqrtl(p);
		weight>>=1;
	}
	return u32(lrintl((long double)(int)range/(1+p)));
}

// TODO: remove cut-copy-paste
template<>
u32 BlockCodec::CompressorModel<NoLimitQWContextModel,PAQ1CountBooster,LogisticMixer>::CalculateSubRange(const std::vector<u8> &src,int bitPos,const std::vector<BlockCodec::Model> &models,u32 range,uint shift)
{
	// ditto
	ASSERT(bitPos>=-1,"Invalid pos");

	std::vector<std::pair<BitCounts,uint>> def;
	if (bitPos==-1) {
		BitCounts bc;
		// All the models have a hit for a zero bit (which it really is!)
		PAQ1CountBooster::Initialize(bc);
		PAQ1CountBooster::Increment(bc,false);
		PAQ1CountBooster::Finalize(bc,shift);
		for (auto &it:models)
			def.push_back(std::make_pair(bc,it.weight));
	} else {
		CreateWeightProfile(def,src,bitPos,models,shift);
	}

	long double p=1;
	uint weight=def[0].second;
	for (auto &it:def) {
		while (weight!=it.second) {
			p=sqrtl(p);
			weight>>=1;
		}
		p=(long double)(int)it.first.c0/p;
		p=(long double)(int)it.first.c1/p;
	}
	while (weight!=1) {
		p=sqrtl(p);
		weight>>=1;
	}
	return u32(lrintl((long double)(int)range/(1+p)));
}

// ---

BlockCodec::BlockCodec(BlockCodecType type,uint shift) :
	_type(type),
	_shift(shift)
{
	ASSERT(_type<=BlockCodecType::TypeLast,"Unknown type");
}

void BlockCodec::SetHeader(const std::vector<u8> &header,uint size)
{
	DecodeHeader(header);
	_rawLength=size;
	_header=EncodeHeaderAndClean(_models);
}

std::vector<u8> BlockCodec::EncodeHeaderAndClean(std::vector<Model> &models)
{
	std::vector<u8> header;

	header.clear();
	ASSERT(_rawLength<65536,"Too long buffer");
	header.push_back(_rawLength);
	header.push_back(_rawLength>>8);
	header.push_back(0);						// empty for now

	// clean up first!
	std::sort(models.begin(),models.end(),[&](const Model &a,const Model &b) {
		if (a.weight>b.weight) return true;
		if (a.weight<b.weight) return false;
		ASSERT(a.model!=b.model,"duplicate model");
		return a.model>b.model;
	});
	ASSERT(models.size(),"empty models");
	while (!models.back().weight) models.pop_back();

	uint weight=models[0].weight;
	uint model=0x100;
	for (auto &it:models) {
		if (weight!=it.weight && model<it.model) weight>>=1;	// free double
		while (weight!=it.weight) {
			header.push_back(model);
			weight>>=1;
		}
		model=it.model;
		header.push_back(model);
	}
	while (weight!=1) {
		header.push_back(model);
		weight>>=1;
	}
	ASSERT(header.size()<=123,"Too long header");
	header[2]=header.size();
	return header;
}

void BlockCodec::DecodeHeader(const std::vector<u8> &header)
{
	ASSERT(header.size()>=4,"Too short header");
	uint size=header[0]+(uint(header[1])<<8);
	ASSERT(size,"null size");
	_rawLength=size;
	_models.clear();

	uint weight=1;
	uint model=0x100;
	for (uint i=3;i<header[2];i++) {
		if (header[i]>=model) {
			for (auto &m:_models) m.weight<<=1;
			if (header[i]==model) continue;
		}
		_models.push_back(Model{header[i],weight});
		model=header[i];
	}
}

void BlockCodec::CreateContextModels(const std::vector<u8> &src,bool multipleRetryPoints,bool multipleInitPoints)
{
	// TODO: this code is messy after few rounds of refactoring...
	const uint numWeights=7;	// try from 1/2^2 to 1/2^8
	uint retryCount=multipleRetryPoints?4:1;

	CreateCompressorModel();

	// too short block is non estimatable block
	if (src.size()<=_cm->GetStartOffset()) {
		_models.clear();
		_models.push_back(Model{0,2});
		_rawLength=src.size();
		_header=EncodeHeaderAndClean(_models);
		_estimatedLength=uint(_header.size()+src.size())*8;
		return;
	}

	// first bytes do not have any modeling and thus can be excluded here
	uint bitLength=uint(src.size()-_cm->GetStartOffset())*8;
	std::vector<std::vector<Probability>> probabilityMap(bitLength);

	TICK();
	DispatchLoop(bitLength,32,[&](size_t i) {
		_cm->CalculateAllProbabilities(probabilityMap[i],src,i+_cm->GetStartOffset()*8,_shift);
	});


	auto StaticLength=[&](std::vector<Model> &models)->float {
		return _cm->GetStartOffset()*8;
	};

	auto IterateModels=[&](std::vector<Model> &models)->std::pair<std::vector<Model>,float> {
		// parallelization makes this a tad more complicated.
		// first loop and then decide the best from the candidates
		uint modelListSize=_cm->GetModelCount()*(numWeights+models.size());
		std::vector<std::pair<std::vector<Model>,float>> modelList(modelListSize);
		for (auto &it:modelList) it.second=std::numeric_limits<float>::infinity();

		DispatchLoop(modelListSize,1,[&](size_t combinedIndex) {
			uint i=combinedIndex/_cm->GetModelCount();
			uint newModel=combinedIndex%_cm->GetModelCount();
			std::vector<Model> testModels=models;
			bool modified=false;
			auto current=std::find_if(testModels.begin(),testModels.end(),[&](const Model &a) {
				return a.model==newModel;
			});
			if (i<numWeights)
			{
				uint weight=2<<i;
				if (current==testModels.end()) {
					// test adding a model
					testModels.push_back(Model{newModel,weight});
					modified=true;
				} else if (testModels.size()!=1 || current->weight!=weight) {
					// test changing a weight
					// if weight is the same, test removing model
					if (current->weight==weight) current->weight=0;
						else current->weight=weight;
					modified=true;
				}
			} else {
				if (current==testModels.end()) {
					// try swapping a model
					i-=numWeights;
					if (testModels[i].model!=newModel) {
						testModels[i].model=newModel;
						modified=true;
					}
				}
			}
			if (modified) {
				float length=0;
				for (uint j=0;j<bitLength;j++)
					length+=_cm->EstimateBitLength(probabilityMap[j],testModels);
				length+=EncodeHeaderAndClean(testModels).size()*8;
				modelList[combinedIndex].first=testModels;
				modelList[combinedIndex].second=length;
			}
		});

		auto ret=std::min_element(modelList.begin(),modelList.end(),[&](const std::pair<std::vector<Model>,float> &a,const std::pair<std::vector<Model>,float> &b) { return a.second<b.second; });
		return *ret;
	};

	// Find ideal context set
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<uint> d(0,_cm->GetModelCount()-1);

	std::vector<std::vector<uint>> seedWeights{
		{},
		{2,2},
		{2,4,8,8},
		{4,4,4,4},
		{4,4,8,8,8,8},
		{4,8,8,8,8,16,16,16,16},
		{16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16}};

	std::vector<Model> currentBestModels;
	float currentBestLength=std::numeric_limits<float>::infinity();
	float staticLength=StaticLength(currentBestModels);

	for (auto &sw:seedWeights) {
		for (uint i=0;i<retryCount;i++) {
			std::vector<Model> testModels=currentBestModels;
			float testLength=std::numeric_limits<float>::infinity();
			for (auto w:sw) {
				uint model=d(mt);
				while (std::find_if(testModels.begin(),testModels.end(),[&](Model &m){return m.model==model;})!=testModels.end())
					model=d(mt);
				testModels.push_back(Model{model,w});
			}
			for (;;) {
				auto best=IterateModels(testModels);
				TICK();
				best.second+=staticLength;
				if (best.second<testLength) {
					testModels=best.first;
					testLength=best.second;
				} else break;
			}
			if (testLength<currentBestLength) {
				currentBestModels=testModels;
				currentBestLength=testLength;
			}
			if (!sw.size()) break;
		}
		if (!multipleInitPoints) break;
	}

	// mark length
	_rawLength=uint(src.size());
	
	// will scale and sort
	_models=currentBestModels;
	_header=EncodeHeaderAndClean(_models);
	_estimatedLength=currentBestLength;
}

void BlockCodec::Encode(const std::vector<u8> &src,ArithEncoder &ec)
{
	uint bitLength=uint(src.size()*8);

	CreateCompressorModel();
	ec.PreEncode(_cm->CalculateSubRange(src,-1,_models,ec.GetRange(),_shift));
	for (uint i=0;i<bitLength;i++)
		ec.Encode(BitAtPos(src,i),_cm->CalculateSubRange(src,i,_models,ec.GetRange(),_shift));
	ec.EndSection(_cm->CalculateSubRange(src,bitLength,_models,ec.GetRange(),_shift));
}

std::vector<u8> BlockCodec::Decode(const std::vector<u8> &header,ArithDecoder &dc)
{
	DecodeHeader(header);

	std::vector<u8> ret(_rawLength);
	uint bitLength=uint(ret.size()*8);

	CreateCompressorModel();
	dc.PreDecode(_cm->CalculateSubRange(ret,-1,_models,dc.GetRange(),_shift));
	for (uint i=0;i<bitLength;i++)
		if (dc.Decode(_cm->CalculateSubRange(ret,i,_models,dc.GetRange(),_shift)))
			ret[i>>3]|=0x80U>>(i&7);
	dc.ProcessEndOfSection(_cm->CalculateSubRange(ret,bitLength,_models,dc.GetRange(),_shift));
	return ret;
}

void BlockCodec::CreateCompressorModel()
{
	switch (_type) {
	case BlockCodecType::Single:
		_cm.reset(new CompressorModel<NoLimitQWContextModel,PAQ1CountBooster,LogisticMixer>());
		break;

	case BlockCodecType::Standard:
		_cm.reset(new CompressorModel<QWContextModel,PAQ1CountBooster,LogisticMixer>());
		break;
	
	default:
		break;
	}
}

std::string BlockCodec::PrintModels() const
{
	std::string ret;
	{
		char tmp[32];
		sprintf(tmp,"Shift=%u ",_shift);
		ret+=tmp;
	}
	for (auto &it:_models) {
		char tmp[32];
		sprintf(tmp,"(1/%u)*0x%02x ",it.weight,it.model);
		ret+=tmp;
	}
	return ret;
}
