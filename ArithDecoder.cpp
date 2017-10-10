/* Copyright (C) Teemu Suutari */

#include <vector>

#include "onekpaq_common.h"
#include "ArithDecoder.hpp"

ArithDecoder::ArithDecoder(const std::vector<u8> &src,DecoderType type) :
	_type(type),
	_src(src)
{
	ASSERT(_type<=DecoderType::TypeLast,"Unknown decoder type");
	Normalize();
}

void ArithDecoder::Normalize()
{
	auto decodeBit=[&]()->bool {
		bool ret=_srcPos>>3<_src.size()&&_src[_srcPos>>3]&0x80U>>(_srcPos&7);
		_srcPos++;
		return ret;
	};

	while (_range<0x4000'0000U) {
		_range<<=1;
		_value<<=1;
		if (_type!=DecoderType::Standard) {
			if (!_srcPos)  ASSERT(!decodeBit(),"Wrong start bit");
			if (_srcPos==6) ASSERT(decodeBit(),"Wrong anchor bit");
			if (_srcPos==7) ASSERT(!decodeBit(),"Wrong filler bit");
		}
		if (decodeBit()) _value++;
	}
}

void ArithDecoder::ProcessEndOfSection(u32 subRange)
{
	DEBUG("range %x, subrange %x, value %x",_range,subRange,_value);
	if (_type==DecoderType::SingleAsm) {
		ASSERT(subRange&&subRange<_range-1,"probability error");

		_range-=subRange;
		ASSERT(_value==_range,"End of section not detected");
		_range=1;
		_value=0;
		Normalize();
	}
	DEBUG("%u bits decoded",_srcPos);
}

void ArithDecoder::PreDecode(u32 subRange)
{
	if (_type!=DecoderType::Standard&&!_hasPreBit) {
		ASSERT(!Decode(subRange),"Wrong purge bit");
		_hasPreBit=true;
	}
}

bool ArithDecoder::Decode(u32 subRange)
{
	DEBUG("range %x, subrange %x, value %x",_range,subRange,_value);
	ASSERT(subRange&&subRange<_range-1,"probability error");

	bool ret;
	if (_type==DecoderType::SingleAsm) {
		_range-=subRange;
		ASSERT(_value!=_range,"End of section detected in middle of stream");
		ASSERT(!(_destPos&7)||_value!=_range+1,"Encoder encoded a symbol we bug on");
		if (_value>=_range) {
			_value-=_range+1;
			// _range=subRange-1 would be correct here.
			_range=subRange;
			ret=false;
		} else ret=true;
	} else {
		_range-=subRange;
		if (_value>=_range) {
			_value-=_range;
			_range=subRange;
			ret=false;
		} else ret=true;
	}

	Normalize();
	_destPos++;
	return ret;
}
