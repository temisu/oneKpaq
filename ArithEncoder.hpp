/* Copyright (C) Teemu Suutari */

#ifndef ARITHENCODER_H
#define ARITHENCODER_H

#include <vector>

#include "onekpaq_common.h"

class ArithEncoder
{
public:
	enum class EncoderType {
		SingleAsm=0,
		MultiAsm,
		Standard,
		TypeLast=Standard
	};

	ArithEncoder(EncoderType type);
	~ArithEncoder() = default;

	u32 GetRange() const { return _range; }
	void PreEncode(u32 subRange);
	void Encode(bool bit,u32 subRange);

	void EndSection(u32 subRange);
	void Finalize();

	const std::vector<u8> &GetDest() const { return _dest; }

private:
	void Normalize();

	EncoderType		_type;
	std::vector<u8>		_dest;
	uint			_destPos=0;
	u32			_range=0x4000'0000U;
	u64			_value=0;
	uint			_bitCount=33;
	bool			_hasPreBit=false;
};

#endif
