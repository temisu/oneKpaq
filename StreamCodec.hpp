/* Copyright (C) Teemu Suutari */

#ifndef STREAMCODEC_H
#define STREAMCODEC_H

#include <vector>
#include <string>

#include "onekpaq_common.h"


class StreamCodec
{
public:
	enum class EncodeMode : uint {
		Single=1,
		Multi,
		SingleFast,
		MultiFast,
		ModeLast=MultiFast
	};

	enum class EncoderComplexity : uint {
		Low=1,
		Medium,
		High,
		ComplexityLast=High
	};

	StreamCodec() { }
	~StreamCodec() = default;

	std::vector<u8> CreateSingleStream();
	const std::vector<u8> &GetAsmDest1() { return _destAsm1; }
	const std::vector<u8> &GetAsmDest2() { return _destAsm2; }
	EncodeMode getMode() const { return _mode; }
	uint GetShift() const { return _shift; }

	void Encode(const std::vector<std::vector<u8>> &blocks,EncodeMode mode,EncoderComplexity complexity,const std::string &cacheFileName);

	void LoadStream(std::vector<u8> singleStream);
	std::vector<u8> Decode();
	std::vector<u8> DecodeAsmStream();

private:
	std::vector<u8> CreateAsmHeader();

	std::vector<std::vector<u8>>	_header;
	std::vector<u8>			_dest;		// encoded stream
	std::vector<u8>			_destAsm1;
	std::vector<u8>			_destAsm2;
	EncodeMode			_mode=EncodeMode::Single;
	uint				_shift;
};


#endif
