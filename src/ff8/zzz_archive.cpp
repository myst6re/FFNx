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

#define CHUNK_SIZE 10485760 // 10 Mio

Zzz::Zzz(const char *fileName) :
	_fileName(fileName), _currentFile(0), _fileCount(0)
{
	_f = fopen(fileName, "rb");
}

Zzz::~Zzz()
{
	if (_f != nullptr)
	{
		fclose(_f);
	}
}

bool Zzz::lookup(const char *fileName, ZzzTocEntry &tocEntry)
{
	if (_f == nullptr)
	{
		return false;
	}

	uint32_t fileCount;

	if (!readHeader(fileCount))
	{
		return false;
	}

	for (uint64_t i = _currentFile; i < fileCount; ++i)
	{
		if (!readTocEntry(tocEntry))
		{
			return false;
		}

		_toc[tocEntry.fileName] = tocEntry;

		if (stricmp(tocEntry.fileName.c_str(), fileName) == 0)
		{
			return true;
		}
	}

	return false;
}

bool Zzz::extractFile(const char *source, const char *target)
{
	ZzzTocEntry tocEntry;
	if (!lookup(source, tocEntry))
	{
		return false;
	}

	// TODO: mkdir -p

	FILE *f = fopen(target, "wb");

	if (f == nullptr)
	{
		return false;
	}

	if (!copyFile(tocEntry, f)) {
		return false;
	}

	return true;
}

Zzz::File *Zzz::openFile(const char *fileName)
{
	ZzzTocEntry tocEntry;
	if (!lookup(fileName, tocEntry))
	{
		return nullptr;
	}

	return new File(tocEntry, *this);
}

void Zzz::closeFile(File *file)
{
	if (file != nullptr)
	{
		delete file;
	}
}

bool Zzz::readHeader(uint32_t &fileCount)
{
	if (_fileCount > 0)
	{
		return _fileCount;
	}

	bool ok = readUInt(fileCount);

	if (ok)
	{
		_fileCount = fileCount;
	}

	return ok;
}

bool Zzz::readTocEntry(ZzzTocEntry &tocEntry)
{
	uint32_t size;

	if (!readUInt(size)) {
		return false;
	}

	char fileName[256];

	if (size > sizeof(fileName)) {
		return false;
	}

	if (fread(fileName, size, 1, _f) != 1) {
		return false;
	}

	tocEntry.fileName = fileName;

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
	// Remember the current pos in the TOC to go back after the file extraction
	long curPos = ftell(_f);

	if (fseek(_f, tocEntry.filePos, SEEK_SET) != 0) {
		return false;
	}

	bool ret = copyFileBuffered(tocEntry, out);

	// Go back to the position in the TOC
	if (fseek(_f, curPos, SEEK_SET) != 0) {
		return false;
	}

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

Zzz::File::File(const ZzzTocEntry &tocEntry, Zzz &parent) :
	_tocEntry(tocEntry), _parent(parent), _relativePos(0)
{
}

bool Zzz::File::seek(uint32_t pos)
{
	if (_parent.seekFile(_tocEntry, pos))
	{
		_relativePos = pos;

		return true;
	}

	return false;
}

int Zzz::File::read(char *data, int size)
{
	int r = _parent.readFile(_tocEntry, _relativePos, data, size);

	if (r > 0)
	{
		_relativePos += r;

		return r;
	}

	return r;
}

uint32_t Zzz::File::relativePos() const
{
	return _relativePos;
}

uint32_t Zzz::File::size() const
{
	return _tocEntry.fileSize;
}

const std::string &Zzz::File::fileName() const
{
	return _tocEntry.fileName;
}
