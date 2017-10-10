/* Copyright (C) Teemu Suutari */

#include <vector>

#include "onekpaq_common.h"
#include "ArithEncoder.hpp"

ArithEncoder::ArithEncoder(EncoderType type) :
	_type(type)
{
	ASSERT(_type<=EncoderType::TypeLast,"Unknown encoder type");
}

void ArithEncoder::Normalize()
{
	auto encodeBit=[&](bool bit) {
		_dest.resize((_destPos>>3)+1);
		if (bit) _dest[_destPos>>3]|=0x80U>>(_destPos&7);
		_destPos++;
	};

	while (_range<0x4000'0000U) {
		if (_bitCount) _bitCount--;
		else {
			if (_type!=EncoderType::Standard) {
				if (!_destPos) encodeBit(false);	// start bit
				if (_destPos==6) encodeBit(true);	// anchor bit
				if (_destPos==7) encodeBit(false);	// filler bit
			}
			ASSERT(!(_value&0x8000'0000'0000'0000ULL),"Overflow");
			encodeBit(_value&0x4000'0000'0000'0000ULL);
			_value&=~0x4000'0000'0000'0000ULL;
		}
		_value<<=1;
		_range<<=1;
	}
}

void ArithEncoder::PreEncode(u32 subRange)
{
	if (_type!=EncoderType::Standard&&!_hasPreBit) {
		Encode(false,subRange);
		_hasPreBit=true;
	}
}

void ArithEncoder::Encode(bool bit,u32 subRange)
{
	DEBUG("bit %u, range %x, subrange %x, value %llx",bit,_range,subRange,_value);
	ASSERT(subRange&&subRange<_range-1,"probability error");

	if (_type==EncoderType::SingleAsm) {
		if (bit) _range-=subRange;
		else {
			_value+=_range-subRange+1;
			// _range=subRange-1 would be correct here.
			_range=subRange;
		}
	} else {
		if (bit) _range-=subRange;
		else {
			_value+=_range-subRange;
			_range=subRange;
		}
	}
	Normalize();
}

void ArithEncoder::EndSection(u32 subRange)
{
	DEBUG("range %x, subrange %x, value %llx",_range,subRange,_value);
	ASSERT(subRange&&subRange<_range-1,"probability error at the end");

	if (_type==EncoderType::SingleAsm) {
		_value+=_range-subRange;
		_range=1;
		Normalize();
	}
}

void ArithEncoder::Finalize()
{
#if 1
	for (uint i=0;i<63;i++) {
		_range>>=1;
		Normalize();
	}
#else
	u64 valueX=_value+_range^_value;
	while (!(valueX&0x4000'0000'0000'0000ULL)) {
		_range>>=1;
		Normalize();
		valueX<<=1;
	}
	_value|=0x4000'0000'0000'0000ULL;
	_range>>=1;
	Normalize();
#endif
	DEBUG("%u bits encoded",_destPos);
}
