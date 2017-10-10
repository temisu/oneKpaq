/* Copyright (C) Teemu Suutari */

#include <fstream>
#include <sys/stat.h>

#include "onekpaq_common.h"

#include "BlockCodec.hpp"
#include "StreamCodec.hpp"
#include "AsmDecode.hpp"

// references used to implement this:
// http://en.wikipedia.org/wiki/PAQ
// http://en.wikipedia.org/wiki/Context_mixing
// http://code4k.blogspot.com/2010/12/crinkler-secrets-4k-intro-executable.html
// clean implementation. no foreign code used.
// compared against paq8i and paq8l.

#define VERIFY_STREAM 1

// sans error handling
std::vector<u8> readFile(const std::string &fileName)
{
	std::vector<u8> ret;
	std::ifstream file(fileName.c_str(),std::ios::in|std::ios::binary|std::ios::ate);
	if (file.is_open()) {
		size_t fileLength=size_t(file.tellg());
		file.seekg(0,std::ios::beg);
		ret.resize(fileLength);
		file.read(reinterpret_cast<char*>(ret.data()),fileLength);
		file.close();
	}
	return ret;
}

// ditto
void writeFile(const std::string &fileName,const std::vector<u8> &src) {
	std::ofstream file(fileName.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (file.is_open()) {
		file.write(reinterpret_cast<const char*>(src.data()),src.size());
		file.close();
	}
}

int main(int argc,char **argv)
{
#ifdef __x86_64__
	INFO("oneKpaq v" ONEKPAQ_VERSION " 64-bit");
#else
	INFO("oneKpaq v" ONEKPAQ_VERSION " 32-bit");
#endif
	if (std::string(argv[0]).find("onekpaq_encode")!=std::string::npos) {
		if (argc<5) ABORT("usage: onekpaq_encode mode complexity block1 block2 ... blockn output.onekpaq");

		StreamCodec::EncodeMode mode=StreamCodec::EncodeMode(atoi(argv[1]));
		StreamCodec::EncoderComplexity complexity=StreamCodec::EncoderComplexity(atoi(argv[2]));

		std::vector<std::vector<u8>> blocks(argc-4);
		for (uint i=0;i<uint(argc-4);i++) {
			blocks[i]=readFile(std::string(argv[i+3]));
			ASSERT(blocks[i].size(),"Empty/missing file");
		}

		struct stat st;
		ASSERT(::stat(argv[argc-1],&st)==-1,"Destination file exists");

		StreamCodec s;
		s.Encode(blocks,mode,complexity,"onekpaq_context.cache");
		auto stream=s.CreateSingleStream();

#ifdef VERIFY_STREAM
		INFO("Verifying...");

		StreamCodec s2;
		s2.LoadStream(stream);
		auto verify=s2.Decode();

		std::vector<u8> src;
		for (auto &it:blocks)
			src.insert(src.end(),it.begin(),it.end());

		ASSERT(src.size()==verify.size(),"size mismatch");

		for (uint i=0;i<src.size();i++)
			ASSERT(src[i]==verify[i],"%u: %02x!=%02x",i,src[i],verify[i]);
		INFO("Data verified");


		INFO("Verifying ASM stream...");
		auto verify2=s.DecodeAsmStream();
		for (uint i=0;i<src.size();i++)
			ASSERT(src[i]==verify2[i],"%u: %02x!=%02x",i,src[i],verify2[i]);
		INFO("ASM Data verified");

		INFO("Using actual ASM-decompressor");
		// now same thing with asm-decoder
		auto asmVerify=AsmDecode(s.GetAsmDest1(),s.GetAsmDest2(),mode,s.GetShift());

		// asm decoder does not know anything about size, we can only verify contents
		for (uint i=0;i<src.size();i++)
			ASSERT(src[i]==asmVerify[i],"%u: %02x!=%02x",i,src[i],asmVerify[i]);
		INFO("ASM-decompressor finished");
#endif

		writeFile(std::string(argv[argc-1]),stream);

	} else if (std::string(argv[0]).find("onekpaq_decode")!=std::string::npos) {
		if (argc!=3) ABORT("usage: onekpaq_decode input.onekpaq output");

		struct stat st;
		ASSERT(::stat(argv[2],&st)==-1,"Destination file exists");

		auto src=readFile(std::string(argv[1]));

		StreamCodec s2;
		s2.LoadStream(src);
		auto dest=s2.Decode();

#if 0
		// now same thing with asm-decoder
		auto asmVerify=AsmDecode(s2.CreateAsmHeader(),s2.GetDest(),s2.getMode());

		// asm decoder does not know anything about size, we can only verify contents
		for (uint i=0;i<dest.size();i++)
			ASSERT(dest[i]==asmVerify[i],"%u: %02x!=%02x",i,dest[i],asmVerify[i]);
		INFO("Data verified");
#endif

		writeFile(std::string(argv[2]),dest);
	} else ABORT("use onekpaq_encode or onekpaq_decode");
	return 0;
}
