/* Copyright (C) Teemu Suutari */

#ifndef CACHEFILE_H
#define CACHEFILE_H

#include <fstream>
#include <vector>
#include <string>

#include "onekpaq_common.h"

class CacheFile {
public:
	CacheFile() { }
	~CacheFile() = default;

	void clear(uint numBlocks);

	uint getNumBlocks() const { return _numBlocks; }
	void setShift(uint shift) { _shift=shift; }
	uint getShift() const { return _shift; }
	std::vector<uint> &getCombineData() { return _combine; }
	std::vector<std::vector<u8>> &getHeader() { return _header; }

	bool readFile(const std::string &fileName);
	bool writeFile(const std::string &fileName);

private:
	uint readUint(std::ifstream &stream);
	std::vector<u8> readVector(std::ifstream &stream);
	void writeUint(std::ofstream &stream,uint value);
	void writeVector(std::ofstream &stream,const std::vector<u8> &vector);

	uint				_shift=0;
	uint				_numBlocks=0;
	std::vector<uint>		_combine;
	std::vector<std::vector<u8>>	_header;
};

#endif
