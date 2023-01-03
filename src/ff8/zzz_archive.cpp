/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2022 myst6re                                            //
//    Copyright (C) 2022 Julian Xhokaxhiu                                   //
//    Copyright (C) 2022 Tang-Tang Zhou                                     //
//                                                                          //
//    This file is part of FFNx                                             //
//                                                                          //
//    FFNx is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU General Public License as published by  //
//    the Free Software Foundation, either version 3 of the License         //
//                                                                          //
//    FFNx is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of        //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
//    GNU General Public License for more details.                          //
/****************************************************************************/
#include "zzz_archive.h"

#include <algorithm>
#include <io.h>
#include <fcntl.h>

#include "../log.h"

#define CHUNK_SIZE 10485760 // 10 Mio

Zzz::Zzz() :
	_f(nullptr)
{
	memset(_fileName, 0, MAX_PATH);
}

Zzz::~Zzz()
{
}

errno_t Zzz::open(const char *fileName)
{
	strncpy(_fileName, fileName, MAX_PATH - 1);
	errno_t err = fopen_s(&_f, fileName, "rb");

	if (err != 0) {
		return err;
	}

	if (!openHeader()) {
		return EILSEQ;
	}

	fclose(_f);

	return 0;
}

bool Zzz::isOpen() const
{
	return !_toc.empty();
}

bool Zzz::lookup(const char *fileName, size_t size, ZzzTocEntry &tocEntry) const
{
	ffnx_trace("Zzz::%s: %s %d\n", __func__, fileName, size);
	char transformedFileName[128];
	size_t sizeToTransform = std::min(size + 1, size_t(128)) - 1;

	for (int i = 0; i < sizeToTransform; ++i) {
		if (fileName[i] == '/') {
			transformedFileName[i] = '\\';
		} else {
			transformedFileName[i] = tolower(fileName[i]);
		}
	}

	transformedFileName[sizeToTransform] = '\0';

	auto it = _toc.find(transformedFileName);

	if (it == _toc.end()) {
		ffnx_error("Zzz::%s: file %s not found\n", __func__, fileName);

		return false;
	}

	tocEntry = it->second;

	return true;
}

bool Zzz::openHeader()
{
	if (!_toc.empty())
	{
		return true;
	}

	uint32_t fileCount;

	if (!readHeader(fileCount))
	{
		ffnx_error("Zzz::%s: cannot read header\n", __func__);

		return false;
	}

	ffnx_info("Zzz::%s: found %d files\n", __func__, fileCount);

	for (uint64_t i = 0; i < fileCount; ++i)
	{
		ZzzTocEntry tocEntry;

		if (!readTocEntry(tocEntry))
		{
			ffnx_error("Zzz::%s: cannot read toc entry %d\n", __func__, i);

			return false;
		}

		ffnx_info("Zzz::%s: %s %d\n", __func__, tocEntry.fileName, tocEntry.fileSize);

		_toc[tocEntry.fileName] = tocEntry;
	}

	return true;
}

Zzz::File *Zzz::openFile(const char *fileName, size_t size) const
{
	int fd = 0;

	ffnx_trace("Zzz::%s: %s %d\n", __func__, fileName, size);

	errno_t err = _sopen_s(&fd, _fileName, _O_BINARY, _SH_DENYNO, _S_IREAD);
	if (err != 0)
	{
		return nullptr;
	}

	ZzzTocEntry tocEntry;
	if (!lookup(fileName, size, tocEntry))
	{
		return nullptr;
	}

	if (_lseeki64(fd, tocEntry.filePos, SEEK_SET) != tocEntry.filePos)
	{
		return nullptr;
	}

	return new File(tocEntry, fd);
}

void Zzz::closeFile(File *file)
{
	if (file != nullptr)
	{
		delete file;
	}
}

const char *Zzz::fileName() const
{
	return _fileName;
}

bool Zzz::readHeader(uint32_t &fileCount)
{
	return readUInt(fileCount);
}

bool Zzz::readTocEntry(ZzzTocEntry &tocEntry)
{
	uint32_t size;

	if (!readUInt(size)) {
		return false;
	}

	char fileName[128];

	if (size >= sizeof(fileName)) {
		return false;
	}

	if (fread(fileName, size, 1, _f) != 1) {
		return false;
	}

	fileName[size] = '\0';

	memcpy(tocEntry.fileName, fileName, size + 1);
	tocEntry.fileNameSize = size;

	uint64_t filePos;
	uint32_t fileSize;

	if (!readULong(filePos)) {
		return false;
	}

	tocEntry.filePos = filePos;

	if (!readUInt(fileSize)) {
		return false;
	}

	tocEntry.fileSize = fileSize;

	return true;
}

bool Zzz::copyFile(const ZzzTocEntry &tocEntry, FILE *out)
{
	errno_t err = fopen_s(&_f, _fileName, "rb");

	if (err != 0) {
		return err;
	}

	if (fseek(_f, tocEntry.filePos, SEEK_SET) != 0) {
		fclose(_f);

		return false;
	}

	bool ret = copyFileBuffered(tocEntry, out);

	fclose(_f);

	return ret;
}

bool Zzz::copyFileBuffered(const ZzzTocEntry &tocEntry, FILE *out)
{
	uint32_t chunkCount = tocEntry.fileSize / CHUNK_SIZE,
	        remaining = tocEntry.fileSize % CHUNK_SIZE;
	char *data = new char[CHUNK_SIZE];

	for (uint64_t i = 0; i < chunkCount; ++i) {
		if (fread(data, CHUNK_SIZE, 1, _f) != 1) {
			delete[] data;
			return false;
		}

		if (fwrite(data, CHUNK_SIZE, 1, out) != 1) {
			delete[] data;
			return false;
		}
	}

	if (remaining > 0) {
		if (fread(data, remaining, 1, _f) != 1) {
			delete[] data;
			return false;
		}

		if (fwrite(data, remaining, 1, out) != 1) {
			delete[] data;
			return false;
		}
	}

	delete[] data;

	return true;
}

bool Zzz::seekFile(const ZzzTocEntry &tocEntry, int relativePos)
{
	return fseek(_f, tocEntry.filePos + relativePos, SEEK_SET);
}

int Zzz::readFile(const ZzzTocEntry &tocEntry, int relativePos, char *data, int size)
{
	if (!seekFile(tocEntry, relativePos))
	{
		return -1;
	}

	size_t ret = fread(data, 1, size, _f);

	if (ferror(_f))
	{
		return -1;
	}

	return ret;
}

bool Zzz::readUInt(uint32_t &val)
{
	return fread(&val, 4, 1, _f) == 1;
}

bool Zzz::readULong(uint64_t &val)
{
	return fread(&val, 8, 1, _f) == 1;
}

Zzz::File::File(const ZzzTocEntry &tocEntry, int fd) :
	_tocEntry(tocEntry), _fd(fd), _pos(tocEntry.filePos)
{
}

Zzz::File::~File()
{
	ffnx_info("Zzz::File::close: %s\n", _tocEntry.fileName);

	_close(_fd);
}

int64_t Zzz::File::seek(int32_t pos, Whence whence)
{
	ffnx_info("Zzz::File::%s: %u\n", __func__, pos);

	switch (whence) {
		case SeekEnd:
			pos = _tocEntry.fileSize + (pos < 0 ? pos : 0);
			break;
		case SeekCur:
			pos += relativePos();
			break;
		case SeekSet:
			break;
	}

	if (pos < 0) {
		return -1;
	}

	_pos = _lseeki64(_fd, _tocEntry.filePos + pos, SEEK_SET);

	ffnx_info("Zzz::File::%s: new pos=%lld filePos=%lld\n", __func__, _pos, _tocEntry.filePos);

	return relativePos();
}

int Zzz::File::read(void *data, unsigned int size)
{
	int64_t pos = relativePos();

	ffnx_info("Zzz::File::%s: size=%u pos=%lld fileSize=%u\n", __func__, size, pos, _tocEntry.fileSize);

	if (pos < 0) {
		return -1; // Before the beginning of the file
	}

	if (pos >= _tocEntry.fileSize) {
		return 0; // End Of File
	}

	int r = _read(_fd, data, std::min(size, uint32_t(_tocEntry.fileSize - pos)));

	ffnx_info("Zzz::File::%s: read %d bytes\n", __func__, r);

	if (r > 0) {
		_pos += r;
	}

	return r;
}

int64_t Zzz::File::relativePos() const
{
	return int64_t(_pos - _tocEntry.filePos);
}

uint64_t Zzz::File::absolutePos() const
{
	return _tocEntry.filePos;
}

uint32_t Zzz::File::size() const
{
	return _tocEntry.fileSize;
}

const char *Zzz::File::fileName() const
{
	return _tocEntry.fileName;
}

uint32_t Zzz::File::fileNameSize() const
{
	return _tocEntry.fileNameSize;
}
