/* Copyright (C) Teemu Suutari */

#ifndef ASMDECODE_H  
#define ASMDECODE_H  

#include <vector>

#include "onekpaq_common.h"

#include "StreamCodec.hpp"

std::vector<u8> AsmDecode(const std::vector<u8> &src1,const std::vector<u8> &src2,StreamCodec::EncodeMode mode,uint shift);

#endif
