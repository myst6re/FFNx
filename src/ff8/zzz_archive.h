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
#pragma once

#include <stdio.h>
#include <map>
#include <string>
#include <windows.h>

struct ZzzTocEntry
{
	uint64_t filePos;
	uint32_t fileSize;
	char fileName[128];
	uint32_t fileNameSize;
};

class Zzz
{
public:
	class File
	{
	public:
		enum Whence {
			SeekSet = SEEK_SET,
			SeekEnd = SEEK_END,
			SeekCur = SEEK_CUR
		};
		int64_t seek(int32_t pos, Whence whence);
		int read(void *data, unsigned int size);
		int64_t relativePos() const;
		uint64_t absolutePos() const;
		uint32_t size() const;
		const char *fileName() const;
		uint32_t fileNameSize() const;
		inline int fd() const {
			return _fd;
		}
	private:
		friend class Zzz;
		File(const ZzzTocEntry &tocEntry, int fd);
		~File();
		const ZzzTocEntry _tocEntry;
		int _fd;
		int64_t _pos;
	};

	Zzz();
	~Zzz();
	errno_t open(const char *fileName);
	bool isOpen() const;
	File *openFile(const char *fileName, size_t size) const;
	static void closeFile(File *file);
	bool lookup(const char *fileName, size_t size, ZzzTocEntry &tocEntry) const;
	const char *fileName() const;
	bool copyFile(const ZzzTocEntry &tocEntry, FILE *out);
private:
	friend class File;
	bool openHeader();
	bool readHeader(uint32_t &fileCount);
	bool readTocEntry(ZzzTocEntry &tocEntry);
	bool copyFileBuffered(const ZzzTocEntry &tocEntry, FILE *out);
	bool readUInt(uint32_t &val);
	bool readULong(uint64_t &val);
	bool seekFile(const ZzzTocEntry &tocEntry, int relativePos);
	int readFile(const ZzzTocEntry &tocEntry, int relativePos, char *data, int size);

	std::map<std::string, ZzzTocEntry> _toc;
	char _fileName[MAX_PATH];
	FILE *_f;
};
