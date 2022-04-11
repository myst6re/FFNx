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

struct ZzzTocEntry
{
	std::string fileName;
	uint64_t filePos;
	uint32_t fileSize;
};

class Zzz
{
public:
	class File
	{
	public:
		bool seek(uint32_t pos);
		int read(char *data, int size);
		uint32_t relativePos() const;
		uint32_t size() const;
		const std::string &fileName() const;
	private:
		friend class Zzz;
		File(const ZzzTocEntry &tocEntry, Zzz &parent);
		const ZzzTocEntry &_tocEntry;
		Zzz &_parent;
		uint32_t _relativePos;
	};

	explicit Zzz(const char *fileName);
	~Zzz();
	bool extractFile(const char *source, const char *target);
	File *openFile(const char *fileName);
	void closeFile(File *file);
private:
	friend class File;
	bool lookup(const char *fileName, ZzzTocEntry &tocEntry);
	bool readHeader(uint32_t &fileCount);
	bool readTocEntry(ZzzTocEntry &tocEntry);
	bool copyFile(const ZzzTocEntry &tocEntry, FILE *out);
	bool copyFileBuffered(const ZzzTocEntry &tocEntry, FILE *out);
	bool readUInt(uint32_t &val);
	bool readULong(uint64_t &val);
	bool seekFile(const ZzzTocEntry &tocEntry, int relativePos);
	int readFile(const ZzzTocEntry &tocEntry, int relativePos, char *data, int size);
	const char *_fileName;
	FILE *_f;
	uint32_t _currentFile;
	uint32_t _fileCount;
	std::map<std::string, ZzzTocEntry> _toc;
};
