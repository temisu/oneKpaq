/* Copyright (C) Teemu Suutari */

#ifndef ARITHDECODER_H  
#define ARITHDECODER_H  

#include <vector>

#include "onekpaq_common.h"

class ArithDecoder
{
public:
	enum class DecoderType {
		SingleAsm=0,
		MultiAsm,
		Standard,
		TypeLast=Standard
	};

	ArithDecoder(const std::vector<u8> &src,DecoderType type);
	~ArithDecoder() = default;

	u32 GetRange() const { return _range; }
	void ProcessEndOfSection(u32 subRange);
	void PreDecode(u32 subRange);
	bool Decode(u32 subRange);

private:
	void Normalize();

	DecoderType			_type;
	const std::vector<u8>&		_src;
	uint				_srcPos=0;
	uint				_destPos=0;
	u32				_value=0;
	u32				_range=1;
	bool				_hasPreBit=false;
};

#endif
