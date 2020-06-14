/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Julian Xhokaxhiu                                   //
//    Copyright (C) 2020 myst6re                                            //
//    Copyright (C) 2020 Chris Rizzitello                                   //
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

#include <filesystem>
#include <string.h>
#include <sys/stat.h>

#include "../ff7.h"
#include "../log.h"
#include "../hext.h"

char* langs_2[5] = {
	"en",
	"fr",
	"de",
	"es",
	"ja"
};

char* langs[5] = {
	"",
	"f",
	"g",
	"s",
	"j"
};

int attempt_redirection(char* in, char* out, size_t size, bool wantsSteamPath = false)
{
	std::string newIn(in);
	
	std::transform(newIn.begin(), newIn.end(), newIn.begin(), ::tolower);

	bool isSavegame = strstr(newIn.data(), ".ff7") != NULL;
	bool isCacheFile = strstr(newIn.data(), ".p") != NULL;

	if (wantsSteamPath && _access(in, 0) == -1)
	{
		if (
			strcmp(newIn.data(), "scene.bin") == 0 ||
			strcmp(newIn.data(), "camdat0.bin") == 0 ||
			strcmp(newIn.data(), "camdat1.bin") == 0 ||
			strcmp(newIn.data(), "camdat2.bin") == 0 ||
			strcmp(newIn.data(), "co.bin") == 0
			)
		{
			get_data_lang_path(out);
			PathAppendA(out, R"(battle)");
			PathAppendA(out, newIn.data());

			if (_access(out, 0) == -1)
				return 1;
		}
		else
		{
			const char* pos = strstr(newIn.data(), "data");

			if (pos != NULL)
			{
				pos += 5;
			}
			else
			{
				// Search for the last '\' character and get a pointer to the next char
				pos = strrchr(in, 92);

				if (pos != NULL) pos += 1;
			}

			get_data_lang_path(out);
			if (pos != NULL) PathAppendA(out, pos);

			if ((_access(out, 0) == -1 || pos == NULL))
			{
				// If steam edition, do one more try in the user data path
				if (steam_edition) get_userdata_path(out, size, isSavegame);
				else strcpy(out, "");

				if (isCacheFile)
				{
					if (steam_edition)
					{
						PathAppendA(out, "cache");
						std::filesystem::create_directories(out);
					}
					PathAppendA(out, newIn.data());
				}
				else
				{
					if (isSavegame)
					{
						pos = strrchr(newIn.data(), 47) + 1;
						PathAppendA(out, pos);
					}
					else
					{
						PathAppendA(out, pos);
						if (_access(out, 0) == -1)
							return 1;
					}
				}
			}
		}

		if (trace_all || trace_files) trace("Redirected: %s -> %s\n", newIn.data(), out);

		return 0;
	}
	else
	{
		if (isSavegame && save_path != nullptr)
		{
			char* pos = strrchr(newIn.data(), 47);

			// This case may happen if we have already redirected the path
			if (strstr(newIn.data(), "save/save") == NULL)
			{
				// Allow the game to continue by forward the redirected path again
				strcpy(out, newIn.data());
			}
			// This one means we still have to redirect it
			else if (pos != NULL)
			{
				strcpy(out, basedir);
				PathAppendA(out, save_path);
				PathAppendA(out, ++pos);

				if (trace_all || trace_files) trace("Redirected: %s -> %s\n", newIn.data(), out);
			}

			// Always return as found in order to allow non existing save files to be saved under the new redirected path
			return 0;
		}
		else if (!isCacheFile)
		{
			const char* pos = strstr(newIn.data(), "data");

			if (pos != NULL)
			{
				pos += 5;
			}
			else
			{
				// Search for the last '\' character and get a pointer to the next char
				pos = strrchr(newIn.data(), 92);

				if (pos != NULL) pos += 1;
			}

			strcpy(out, basedir);
			PathAppendA(out, override_path);
			if (pos != NULL)
				PathAppendA(out, pos);
			else
			{
				PathAppendA(out, R"(battle)");
				PathAppendA(out, newIn.data());
			}

			if (_access(out, 0) == -1)
				return -1;

			if (trace_all || trace_files) trace("Redirected: %s -> %s\n", newIn.data(), out);

			return 0;
		}
	}

	return -1;
}

int attempt_other_languages(char* in, char* out, size_t size)
{
	if (_access(in, 0) != -1)
	{
		strcpy(out, in);
		return 0;
	}

	std::string newIn(in);

	std::transform(newIn.begin(), newIn.end(), newIn.begin(), ::tolower);

	std::equal(newIn.begin(), newIn.end(), "_");

	std::string::size_type pos_filename = newIn.rfind('/');

	if (pos_filename == std::string::npos) {
		pos_filename = newIn.rfind('\\');

		if (pos_filename == std::string::npos) {
			return -1;
		}
	}

	pos_filename += 1;

	std::string::size_type pos_lang = newIn.find('_', pos_filename);

	if (pos_lang == std::string::npos) {
		char* flevel = "flevel", *chocobo = "chocobo", *condor = "condor";
		std::string::size_type pos = newIn.find(flevel, pos_filename);

		if (pos == std::string::npos) {
			pos = newIn.find(chocobo, pos_filename);
			flevel = chocobo;

			if (pos == std::string::npos) {
				pos = newIn.find(condor, pos_filename);
				flevel = condor;

				if (pos == std::string::npos) {
					return -1;
				}
			}
		}

		strncpy(out, newIn.data(), pos_filename);

		for (int i = 0; i < 5; ++i) {
			strcpy(out + pos_filename, langs[i]);
			strcpy(out + pos_filename + strlen(langs[i]), flevel);
			strcpy(out + pos_filename + strlen(langs[i]) + strlen(flevel), ".lgp");

			if (_access(out, 0) != -1)
			{
				return 0;
			}
		}
	}

	pos_lang += 1;

	strncpy(out, newIn.data(), pos_lang);

	for (int i = 0; i < 5; ++i) {
		strcpy(out + pos_lang, langs_2[i]);
		strcpy(out + pos_lang + strlen(langs_2[i]), ".lgp");

		if (_access(out, 0) != -1)
		{
			return 0;
		}
	}

	return -1;
}

FILE *open_lgp_file(char *filename, uint mode)
{
	char _filename[260]{ 0 };
	if(trace_all || trace_files) trace("opening lgp file %s (mode: %i)\n", filename, mode);

	int redirect_status = attempt_redirection(filename, _filename, sizeof(_filename));

	if (redirect_status == -1)
	{
		int status = attempt_other_languages(filename, _filename, sizeof(_filename));

		if (status == -1)
		{
			strcpy(_filename, filename);
		}
	}

	return fopen(_filename, "rb");
}

void close_lgp_file(FILE *fd)
{
	if(!fd) return;

	if(trace_all || trace_files) trace("closing lgp file\n");

	fclose(fd);
}

// LGP names used for modpath lookup
char lgp_names[18][256] = {
	"char",
	"flevel",
	"battle",
	"magic",
	"menu",
	"world",
	"condor",
	"chocobo",
	"high",
	"coaster",
	"snowboard",
	"midi",
	"",
	"",
	"moviecam",
	"cr",
	"disc",
	"sub",
};

struct lgp_file
{
	uint is_lgp_offset;
	union
	{
		uint offset;
		FILE *fd;
	};
	uint resolved_conflict;
};

#define NUM_LGP_FILES 64

struct lgp_file *lgp_files[NUM_LGP_FILES];
uint lgp_files_index = 0;

struct lgp_file *last;

char lgp_current_dir[256];

uint use_files_array = true;

int lgp_lookup_value(unsigned char c)
{
	c = tolower(c);
	
	if(c == '.') return -1;
	
	if(c < 'a' && c >= '0' && c <= '9') c += 'a' - '0';
	
	if(c == '_') c = 'k';
	if(c == '-') c = 'l';
	
	return c - 'a';
}

uint lgp_chdir(char *path)
{
	uint len = strlen(path);

	while(path[0] == '/' || path[0] == '\\') path++;
	
	memcpy(lgp_current_dir, path, len + 1);

	while(lgp_current_dir[len - 1] == '/' || lgp_current_dir[len - 1] == '\\') len--;
	lgp_current_dir[len] = 0;

	return true;
}

// original LGP open file logic, unchanged except for the LGP Tools safety net
uint original_lgp_open_file(char *filename, uint lgp_num, struct lgp_file *ret)
{
	uint lookup_value1 = lgp_lookup_value(filename[0]);
	uint lookup_value2 = lgp_lookup_value(filename[1]) + 1;
	struct lookup_table_entry *lookup_table = ff7_externals.lgp_lookup_tables[lgp_num];
	uint toc_offset = lookup_table[lookup_value1 * 30 + lookup_value2].toc_offset;
	uint i;

	// did we find anything in the lookup table?
	if(toc_offset)
	{
		uint num_files = lookup_table[lookup_value1 * 30 + lookup_value2].num_files;

		// look for our file
		for(i = 0; i < num_files; i++)
		{
			struct lgp_toc_entry *toc_entry = &ff7_externals.lgp_tocs[lgp_num * 2][toc_offset + i - 1];

			if(!_stricmp(toc_entry->name, filename))
			{
				if(!toc_entry->conflict)
				{
					// this is the only file with this name, we're done here
					ret->is_lgp_offset = true;
					ret->offset = toc_entry->offset;
					return true;
				}
				else
				{
					struct conflict_list *conflict = &ff7_externals.lgp_folders[lgp_num].conflicts[toc_entry->conflict - 1];
					struct conflict_entry *conflict_entries = conflict->conflict_entries;
					uint num_conflicts = conflict->num_conflicts;
					
					// there are multiple files with this name, look for our
					// current directory in the conflict table
					for(i = 0; i < num_conflicts; i++)
					{
						if(!_stricmp(conflict_entries[i].name, lgp_current_dir))
						{
							struct lgp_toc_entry *toc_entry = &ff7_externals.lgp_tocs[lgp_num * 2][conflict_entries[i].toc_index];

							// file name and directory matches, this is our file
							ret->is_lgp_offset = true;
							ret->offset = toc_entry->offset;
							ret->resolved_conflict = true;
							return true;
						}
					}

					break;
				}
			}
		}
	}

	// one last chance, the lookup table might have been broken by LGP Tools,
	// search through the entire archive
	for(i = 0; i < ((uint *)ff7_externals.lgp_tocs)[lgp_num * 2 + 1]; i++)
	{
		struct lgp_toc_entry *toc_entry = &ff7_externals.lgp_tocs[lgp_num * 2][i];

		if(!_stricmp(toc_entry->name, filename))
		{
			glitch("broken LGP file (%s), don't use LGP Tools!\n", lgp_names[lgp_num]);

			if(!toc_entry->conflict)
			{
				ret->is_lgp_offset = true;
				ret->offset = toc_entry->offset;
				return true;
			}
		}
	}

	return false;
}

// new LGP open file logic with modpath and direct mode support
struct lgp_file *lgp_open_file(char *filename, uint lgp_num)
{
	struct lgp_file *ret = (lgp_file*)external_calloc(sizeof(*ret), 1);
	char tmp[512 + sizeof(basedir)];
	char _fname[_MAX_FNAME];
	char *fname = _fname;
	char ext[_MAX_EXT];
	char name[_MAX_FNAME + _MAX_EXT];

	_splitpath(filename, 0, 0, fname, ext);

	if(strlen(direct_mode_path) > 0)
	{
		_snprintf(tmp, sizeof(tmp), "%s/%s/%s/%s%s", basedir, direct_mode_path, lgp_names[lgp_num], fname, ext);
		ret->fd = fopen(tmp, "rb");

		if(!ret->fd)
		{
			_snprintf(tmp, sizeof(tmp), "%s/%s/%s/%s/%s%s", basedir, direct_mode_path, lgp_names[lgp_num], lgp_current_dir, fname, ext);
			ret->fd = fopen(tmp, "rb");
			if(ret->fd) ret->resolved_conflict = true;
		}

		if(trace_all || trace_direct) trace("lgp_open_file: %i, %s (%s) = 0x%x\n", lgp_num, filename, lgp_current_dir, ret);
	}

	if(!ret->fd)
	{
		sprintf(name, "%s%s", fname, ext);

		if(!original_lgp_open_file(name, lgp_num, ret))
		{
			if(strlen(direct_mode_path) > 0) error("failed to find file %s; tried %s/%s/%s, %s/%s/%s/%s, %s/%s (LGP) (path: %s)\n", filename, direct_mode_path, lgp_names[lgp_num], name, direct_mode_path, lgp_names[lgp_num], lgp_current_dir, name, lgp_names[lgp_num], name, lgp_current_dir);
			else error("failed to find file %s/%s (LGP) (path: %s)\n", lgp_names[lgp_num], name, lgp_current_dir);
			external_free(ret);
			return 0;
		}
	}

	last = ret;

	if(use_files_array && !ret->is_lgp_offset)
	{
		if(lgp_files[lgp_files_index])
		{
			fclose(lgp_files[lgp_files_index]->fd);
			external_free(lgp_files[lgp_files_index]);
		}

		lgp_files[lgp_files_index] = ret;
		lgp_files_index = (lgp_files_index + 1) % NUM_LGP_FILES;
	}

	return ret;
}

/* 
 * Direct LGP file access routines are used all over the place despite the nice
 * generic file interface found below in this file. Therefore we must implement
 * these in a way that works with the original code.
 */

// seek to given offset in LGP file
uint lgp_seek_file(uint offset, uint lgp_num)
{
	if(!ff7_externals.lgp_fds[lgp_num]) return false;

	fseek(ff7_externals.lgp_fds[lgp_num], offset, SEEK_SET);

	return true;
}

// read straight from LGP file
uint lgp_read(uint lgp_num, char *dest, uint size)
{
	if(!ff7_externals.lgp_fds[lgp_num]) return 0;

	if(last->is_lgp_offset) return fread(dest, 1, size, ff7_externals.lgp_fds[lgp_num]);

	return fread(dest, 1, size, last->fd);
}

// read from LGP file by LGP file descriptor
uint lgp_read_file(struct lgp_file *file, uint lgp_num, char *dest, uint size)
{
	if(!ff7_externals.lgp_fds[lgp_num]) return 0;

	if(file->is_lgp_offset)
	{
		lgp_seek_file(file->offset + 24, lgp_num);
		return fread(dest, 1, size, ff7_externals.lgp_fds[lgp_num]);
	}

	return fread(dest, 1, size, file->fd);
}

// retrieve the size of a file within the LGP archive
uint lgp_get_filesize(struct lgp_file *file, uint lgp_num)
{
	if(file->is_lgp_offset)
	{
		uint size;

		lgp_seek_file(file->offset + 20, lgp_num);
		fread(&size, 4, 1, ff7_externals.lgp_fds[lgp_num]);
		return size;
	}
	else
	{
		struct stat s;

		fstat(_fileno(file->fd), &s);

		return s.st_size;
	}
}

// close a file handle
void close_file(struct ff7_file *file)
{
	if(!file) return;

	if(file->fd)
	{
		if(!file->fd->is_lgp_offset && file->fd->fd) fclose(file->fd->fd);
		external_free(file->fd);
	}

	external_free(file->name);
	external_free(file);
}

// open file handle, target could be a file within an LGP archive or a regular
// file on disk
struct ff7_file *open_file(struct file_context *file_context, char *filename)
{
	char mangled_name[200];
	struct ff7_file *ret = (ff7_file*)external_calloc(sizeof(*ret), 1);
	char _filename[260]{ 0 };
	char _newFilename[260]{ 0 };
	int redirect_status = 0;

	if (!ret) return 0;

	if(trace_all || trace_files)
	{
		if(file_context->use_lgp) trace("open %s (LGP:%s)\n", filename, lgp_names[file_context->lgp_num]);
		else trace("open %s (mode %i)\n", filename, file_context->mode);
	}

	
	if (!file_context->use_lgp)
	{
		// Attempt another redirection based on Steam/eStore logic
		int redirect_status = attempt_redirection(filename, _newFilename, sizeof(_newFilename), steam_edition || estore_edition);

		// File was found
		if (redirect_status == 0)
		{
			// Attemp override redirection on top of Steam/eStore new path
			redirect_status = attempt_redirection(_newFilename, _filename, sizeof(_filename));

			if (redirect_status == -1)
			{
				// If was not found, use original redirected path
				strcpy(_filename, _newFilename);
			}
		}
		// File was not found
		else if (redirect_status == -1)
		{
			// Attemp override redirection on top of classic path
			redirect_status = attempt_redirection(filename, _filename, sizeof(_filename));

			if (redirect_status == -1)
			{
				// If was not found, use original filename
				strcpy(_filename, filename);
			}

		}
		// File was not found, but was required
		else if (redirect_status == 1)
		{
			strcpy(_filename, filename);

			goto error;
		}
	}
	else
		// LGP files can be loaded safely from data, as Steam/eStore does not override them
		strcpy(_filename, filename);

	ret->name = (char*)external_malloc(strlen(_filename) + 1);
	strcpy(ret->name, _filename);
	memcpy(&ret->context, file_context, sizeof(*file_context));

	// file name mangler used mainly by battle module to convert PSX file names
	// to LGP-friendly PC names
	if(file_context->name_mangler)
	{
		file_context->name_mangler(_filename, mangled_name);
		strcpy(_filename, mangled_name);

		if(trace_all || trace_files) trace("mangled name: %s\n", mangled_name);
	}

	if(file_context->use_lgp)
	{
		use_files_array = false;
		ret->fd = lgp_open_file(_filename, ret->context.lgp_num);
		use_files_array = true;
		if(!ret->fd)
		{
			if(file_context->name_mangler) error("offset error: %s %s\n", _filename, mangled_name);
			else error("offset error: %s\n", _filename);
			goto error;
		}

		if(!lgp_seek_file(ret->fd->offset + 24, ret->context.lgp_num))
		{
			error("seek error: %s\n", _filename);
			goto error;
		}
	}
	else
	{
		ret->fd = (lgp_file*)external_calloc(sizeof(*ret->fd), 1);

		if(ret->context.mode == FF7_FMODE_READ) ret->fd->fd = fopen(_filename, "rb");
		else if(ret->context.mode == FF7_FMODE_READ_TEXT) ret->fd->fd = fopen(_filename, "r");
		else if(ret->context.mode == FF7_FMODE_WRITE) ret->fd->fd = fopen(_filename, "wb");
		else if(ret->context.mode == FF7_FMODE_CREATE) ret->fd->fd = fopen(_filename, "w+b");
		else ret->fd->fd = fopen(_filename, "r+b");

		if(!ret->fd->fd) goto error;
	}

	return ret;

error:
	// it's normal for save files to be missing, anything else is probably
	// going to cause trouble
	if(file_context->use_lgp || _stricmp(&_filename[strlen(_filename) - 4], ".ff7")) error("could not open file %s\n", _filename);
	close_file(ret);
	return 0;
}

// read from file handle, returns how many bytes were actually read
uint __read_file(uint count, void *buffer, struct ff7_file *file)
{
	uint ret = 0;

	if(!file || !count) return false;

	if(trace_all || trace_files) trace("reading %i bytes from %s (ALT)\n", count, file->name);

	if(file->context.use_lgp) return lgp_read(file->context.lgp_num, (char*)buffer, count);

	ret = fread(buffer, 1, count, file->fd->fd);

	if(ferror(file->fd->fd))
	{
		error("could not read from file %s (%i)\n", file->name, ret);
		return -1;
	}

	return ret;
}

// read from file handle, returns true if the read succeeds
uint read_file(uint count, void *buffer, struct ff7_file *file)
{
	uint ret = 0;

	if(!file || !count) return false;

	if(trace_all || trace_files) trace("reading %i bytes from %s\n", count, file->name);

	if(file->context.use_lgp) return lgp_read(file->context.lgp_num, (char*)buffer, count);

	ret = fread(buffer, 1, count, file->fd->fd);

	if(ret != count)
	{
		error("could not read from file %s (%i)\n", file->name, ret);
		return false;
	}

	return true;
}

// read directly from a file descriptor returned by the open_file function
uint __read(FILE *file, char *buffer, uint count)
{
	return fread(buffer, 1, count, file);
}

// write to file handle, returns true if the write succeeds
uint write_file(uint count, void *buffer, struct ff7_file *file)
{
	uint ret = 0;
	void *tmp = 0;

	if(!file || !count) return false;

	if(file->context.use_lgp) return false;

	if(trace_all || trace_files) trace("writing %i bytes to %s\n", count, file->name);

	// hack to emulate win95 style writes, a NULL buffer means we should write
	// all zeroes
	if(!buffer)
	{
		tmp = driver_calloc(count, 1);
		buffer = tmp;
	}

	ret = fwrite(buffer, 1, count, file->fd->fd);

	if(tmp) driver_free(tmp);

	if(ret != count)
	{
		error("could not write to file %s\n", file->name);
		return false;
	}

	return true;
}

// retrieve the size of a file from file handle
uint get_filesize(struct ff7_file *file)
{
	if(!file) return 0;

	if(trace_all || trace_files) trace("get_filesize %s\n", file->name);

	if(file->context.use_lgp) return lgp_get_filesize(file->fd, file->context.lgp_num);
	else
	{
		struct stat s;
		fstat(_fileno(file->fd->fd), &s);

		return s.st_size;
	}
}

// retrieve the current seek position from file handle
uint tell_file(struct ff7_file *file)
{
	if(!file) return 0;

	if(trace_all || trace_files) trace("tell %s\n", file->name);

	if(file->context.use_lgp) return 0;

	return ftell(file->fd->fd);
}

// seek to position in file
void seek_file(struct ff7_file *file, uint offset)
{
	if(!file) return;

	if(trace_all || trace_files) trace("seek %s to %i\n", file->name, offset);

	// it's not possible to seek within LGP archives
	if(file->context.use_lgp) return;

	if(fseek(file->fd->fd, offset, SEEK_SET)) error("could not seek file %s\n", file->name);
}

// construct modpath name from file context, file handle and filename
char *make_pc_name(struct file_context *file_context, struct ff7_file *file, char *filename)
{
	uint i, len;
	char *backslash;
	char* ret = (char*)external_malloc(1024);

	if(file_context->use_lgp)
	{
		if(file->fd->resolved_conflict) len = _snprintf(ret, 1024, "%s/%s/%s", lgp_names[file_context->lgp_num], lgp_current_dir, filename);
		else len = _snprintf(ret, 1024, "%s/%s", lgp_names[file_context->lgp_num], filename);
	}
	else len = _snprintf(ret, 1024, "%s", filename);

	for(i = 0; i < len; i++)
	{
		if(ret[i] == '.')
		{
			if(!_stricmp(&ret[i], ".tex")) ret[i] = 0;
			else if(!_stricmp(&ret[i], ".p")) ret[i] = 0;
			else ret[i] = '_';
		}
	}

	while(backslash = strchr(ret, '\\')) *backslash = '/';

	return ret;
}
