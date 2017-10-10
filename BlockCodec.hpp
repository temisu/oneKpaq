/* Copyright (C) Teemu Suutari */

#ifndef BLOCKCODEC_H
#define BLOCKCODEC_H

#include <string>
#include <vector>
#include <memory>

#include "onekpaq_common.h"

#include "ArithEncoder.hpp"
#include "ArithDecoder.hpp"


class BlockCodec
{
public:
	enum class BlockCodecType {
		Single=0,
		Standard,
		TypeLast=Standard
	};

	struct Probability;
	struct BitCounts;

private:
	struct Model
	{
		uint			model;
		uint			weight;
	};

	template<class CM,class CB,class MIX>
	class CompressorModel;

	class CompressorModelBase
	{
	public:
		CompressorModelBase() = default;
		virtual ~CompressorModelBase() = default;

		virtual uint GetModelCount() const = 0;
		virtual uint GetStartOffset() const = 0;

		virtual void CalculateAllProbabilities(std::vector<Probability> &dest,const std::vector<u8> &src,uint bitPos,uint shift) = 0;
		virtual float EstimateBitLength(const std::vector<Probability> &probabilities,const std::vector<Model> &models) = 0;

		virtual u32 CalculateSubRange(const std::vector<u8> &src,int bitPos,const std::vector<Model> &models,u32 range,uint shift) = 0;

	private:
		virtual void CreateWeightProfile(std::vector<std::pair<BitCounts,uint>> &dest,const std::vector<u8> &src,uint bitPos,const std::vector<Model> &models,uint shift) = 0;
	};

public:
	BlockCodec(BlockCodecType type,uint shift);
	~BlockCodec() = default;

	const std::vector<u8> &GetHeader() const { return _header; }
	void SetHeader(const std::vector<u8> &header,uint size);

	void CreateContextModels(const std::vector<u8> &src,bool multipleRetryPoints,bool multipleInitPoints);
	void Encode(const std::vector<u8> &src,ArithEncoder &ec);
	uint GetRawLength() const { return _rawLength; }
	float GetEstimatedLength() const { return _estimatedLength; }
	std::string PrintModels() const;

	std::vector<u8> Decode(const std::vector<u8> &header,ArithDecoder &dc);

private:
	std::vector<u8> EncodeHeaderAndClean(std::vector<Model> &models);
	void DecodeHeader(const std::vector<u8> &header);
	void CreateCompressorModel();

	BlockCodecType			_type;
	std::vector<Model> 		_models;		// models, for encoding and decoding
	uint				_rawLength=0;		// unencoded data length
	
	std::vector<u8>			_header;		// for encoding only
	float				_estimatedLength=0;	// with header, encoding only

	uint				_shift;			// shift for encoding and decoding

	std::unique_ptr<CompressorModelBase> _cm;
};

#endif
