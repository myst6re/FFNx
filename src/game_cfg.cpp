/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2023 myst6re                                            //
//    Copyright (C) 2025 Julian Xhokaxhiu                                   //
//    Copyright (C) 2023 Tang-Tang Zhou                                     //
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

#include "game_cfg.h"

#include "patch.h"
#include "globals.h"
#include "cfg.h"
#include "log.h"

void normalize_path_win(char *name)
{
	int idx = 0;
	while (name[idx] != 0)
	{
		if (name[idx] == '/') name[idx] = '\\';
		idx++;
	}
}

void ff8_set_game_paths(int install_options, char *_app_path, const char *_dataDrive)
{
	char fileName[MAX_PATH] = {};

	if (!app_path.empty())
	{
		ffnx_info("Overriding AppPath with %s\n", app_path.c_str());
		strncpy(fileName, app_path.c_str(), sizeof(fileName));
		_app_path = fileName;
		normalize_path_win(_app_path);
	}

	if (!steam_edition && !data_drive.empty())
	{
		ffnx_info("Overriding DataDrive with %s\n", data_drive.c_str());
		_dataDrive = data_drive.c_str();
	}

	ff8_externals.set_game_paths(install_options, _app_path, _dataDrive);
}

int ff8_reg_get_midiguid(LPDWORD midi_guid)
{
	int ret = ff8_externals.reg_get_midiguid((LPBYTE)midi_guid);
	LPDWORD default_midi_guid[4] = {0, 0, 0, 0};

	if (memcmp(midi_guid, default_midi_guid, 0x10u) == 0) {
		ffnx_info("MIDI GUID is zero, force to Microsoft Synthesizer\n");
		// Use default Microsoft synthesizer, instead of starting FF8Config.exe
		uint32_t buf[4] = {0x58C2B4D0, 0x11D146E7, 0xA000AC89, 0x294105C9};
		memcpy(midi_guid, buf, 16);

		return 1;
	}

	return ret;
}

int ff8_reg_get_graphics()
{
	int ret = ff8_externals.reg_get_graphics();

	ret |= 0x100000; // Force this flag to prevent graphical glitches (see FF8.reg)

	return ret;
}

void game_cfg_init()
{
	if (ff8)
	{
		replace_call(ff8_externals.init_config + 0x3E, ff8_set_game_paths);
		replace_call(ff8_externals.init_config + 0x48, ff8_reg_get_midiguid);
		replace_call(ff8_externals.init_config + 0x16B, ff8_reg_get_graphics);
	}
}
