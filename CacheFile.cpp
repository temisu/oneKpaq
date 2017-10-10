/* Copyright (C) Teemu Suutari */

#include "onekpaq_common.h"

#include "CacheFile.hpp"

void CacheFile::clear(uint numBlocks)
{
	_numBlocks=numBlocks;
	_combine.clear();
	_header.clear();
}

bool CacheFile::readFile(const std::string &fileName)
{
	bool ret=false;
	std::ifstream file(fileName.c_str(),std::ios::in|std::ios::binary);
	if (file.is_open()) {
		_shift=readUint(file);
		_numBlocks=readUint(file);
		uint numCombinedBlocks=readUint(file);
		_combine.clear();
		_header.clear();
		for (uint i=0;file&&i<numCombinedBlocks;i++) {
			_combine.push_back(readUint(file));
			_header.push_back(readVector(file));
		}
		ret=bool(file);
		file.close();
	}
	return ret;
}

bool CacheFile::writeFile(const std::string &fileName)
{
	bool ret=false;
	std::ofstream file(fileName.c_str(),std::ios::out|std::ios::binary|std::ios::trunc);
	if (file.is_open()) {
		writeUint(file,_shift);
		writeUint(file,_numBlocks);
		writeUint(file,uint(_header.size()));
		for (uint i=0;file&&i<_header.size();i++) {
			writeUint(file,_combine[i]);
			writeVector(file,_header[i]);
		}
		ret=bool(file);
		file.close();
	}
	return ret;
}

uint CacheFile::readUint(std::ifstream &stream)
{
	if (!stream) return 0;

	// x86 only
	u32 ret=0;
	stream.read(reinterpret_cast<char*>(&ret),sizeof(ret));
	return ret;
}

std::vector<u8> CacheFile::readVector(std::ifstream &stream)
{
	std::vector<u8> ret;
	uint length=readUint(stream);
	if (stream&&length) {
		ret.resize(length);
		stream.read(reinterpret_cast<char*>(ret.data()),length);
	}
	return ret;
}

void CacheFile::writeUint(std::ofstream &stream,uint value)
{
	if (!stream) return;
		
	// x86 only
	u32 tmp=value;
	stream.write(reinterpret_cast<const char*>(&tmp),sizeof(tmp));
}

void CacheFile::writeVector(std::ofstream &stream,const std::vector<u8> &vector)
{
	writeUint(stream,uint(vector.size()));
	if (stream&&vector.size()) stream.write(reinterpret_cast<const char*>(vector.data()),vector.size());
}
