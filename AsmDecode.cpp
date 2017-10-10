/* Copyright (C) Teemu Suutari */

#include <vector>
#include <sys/mman.h>
#include <mach/vm_param.h>

#include "onekpaq_common.h"
#include "AsmDecode.hpp"
#include "Timer.hpp"

extern "C" void *onekpaq_decompressor_mode1;
extern "C" void *onekpaq_decompressor_mode2;
extern "C" void *onekpaq_decompressor_mode3;
extern "C" void *onekpaq_decompressor_mode4;

extern "C" void *onekpaq_decompressor_mode1_shift;
extern "C" void *onekpaq_decompressor_mode2_shift;
extern "C" void *onekpaq_decompressor_mode3_shift;
extern "C" void *onekpaq_decompressor_mode4_shift;

extern "C" void *onekpaq_decompressor_mode1_end;
extern "C" void *onekpaq_decompressor_mode2_end;
extern "C" void *onekpaq_decompressor_mode3_end;
extern "C" void *onekpaq_decompressor_mode4_end;

// Please be PIC!
static void *makeCodeblock(const void *prog,ulong size)
{
	// not executable yet!
	void *ret=mmap(nullptr,(size+PAGE_SIZE-1)&~(PAGE_SIZE-1),PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE,-1,0);
	ASSERT(ret!=MAP_FAILED,"mmap failed");
	memcpy(ret,prog,size);
	return ret;
}


static void makeRunnable(void *ptr,ulong size)
{
	mprotect(ptr,(size+PAGE_SIZE-1)&~(PAGE_SIZE-1),PROT_READ|PROT_EXEC);
}


std::vector<u8> AsmDecode(const std::vector<u8> &src1,const std::vector<u8> &src2,StreamCodec::EncodeMode mode,uint shift) {
	typedef void (*AsmDecompressor)(void*,void*);
	void *decompressors[4]={
		&onekpaq_decompressor_mode1,
		&onekpaq_decompressor_mode2,
		&onekpaq_decompressor_mode3,
		&onekpaq_decompressor_mode4
	};

	ulong decompressorShiftOffsets[4]={
		(ulong)&onekpaq_decompressor_mode1_shift-(ulong)(void*)&onekpaq_decompressor_mode1,
		(ulong)&onekpaq_decompressor_mode2_shift-(ulong)(void*)&onekpaq_decompressor_mode2,
		(ulong)&onekpaq_decompressor_mode3_shift-(ulong)(void*)&onekpaq_decompressor_mode3,
		(ulong)&onekpaq_decompressor_mode4_shift-(ulong)(void*)&onekpaq_decompressor_mode4
	};

	ulong decompressorSizes[4]={
		(ulong)&onekpaq_decompressor_mode1_end-(ulong)(void*)&onekpaq_decompressor_mode1,
		(ulong)&onekpaq_decompressor_mode2_end-(ulong)(void*)&onekpaq_decompressor_mode2,
		(ulong)&onekpaq_decompressor_mode3_end-(ulong)(void*)&onekpaq_decompressor_mode3,
		(ulong)&onekpaq_decompressor_mode4_end-(ulong)(void*)&onekpaq_decompressor_mode4
	};

	// we have no way of calculating the real size for dest without decompressing.
	// lets just put some huge number here
	uint expectedSize=262144;

	std::vector<u8> ret(expectedSize);
	std::vector<u8> combine=src1;
	combine.insert(combine.end(),src2.begin(),src2.end());
	for (int i=0;i<4;i++) combine.push_back(0);
	const uint destStartMargin=9;
	
	uint dIndex=static_cast<uint>(mode)-1;
	AsmDecompressor decompr=(AsmDecompressor)makeCodeblock(decompressors[dIndex],decompressorSizes[dIndex]);
	((u8*)(void*)decompr)[decompressorShiftOffsets[dIndex]]=shift;
	makeRunnable((void*)decompr,decompressorSizes[dIndex]);

	INFO("Running asm decompressor Xbx=%p Xdi=%p",combine.data()+src1.size(),ret.data()+destStartMargin);
	auto timeTaken=Timer([&]() {
		decompr(combine.data()+src1.size(),ret.data()+destStartMargin);
	});
	INFO("Asm decompressor done. Time taken %f seconds",float(timeTaken));

	return std::vector<u8>(ret.begin()+destStartMargin,ret.begin()+expectedSize-destStartMargin);
}
