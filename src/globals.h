/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 myst6re                                            //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2022 Julian Xhokaxhiu                                   //
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

#include <fstream>
#include <windows.h>
#include <toml++/toml.h>

#if defined(__cplusplus)
extern "C" {
#endif

extern const GUID IID_ICodecAPI = { 0x901db4c7, 0x31ce, 0x41a2, 0x85,0xdc, 0x8f,0xa0,0xbf,0x41,0xb8,0xda };

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#if defined(__cplusplus)
}
#endif

#include "utils.h"

#include "ff7.h"
#include "ff7/defs.h"
#include "ff8.h"
#include "ff8/vram.h"

#define FFNX_API __declspec(dllexport)

extern HINSTANCE gameHinstance;
extern HWND gameHwnd;

extern MEMORYSTATUSEX last_ram_state;
extern uint32_t version;
extern uint32_t steam_edition;
extern uint32_t estore_edition;
extern uint32_t ff7_japanese_edition;
extern uint32_t ff7_center_fields;
extern DWORD ff7_sfx_volume;
extern DWORD ff7_music_volume;
extern uint32_t ff7_do_reset;

#define BASEDIR_LENGTH 512
extern char basedir[BASEDIR_LENGTH];

extern uint32_t game_width;
extern uint32_t game_height;
extern uint32_t x_offset;
extern uint32_t y_offset;

extern struct texture_format *texture_format;

extern struct ff7_externals ff7_externals;
extern struct ff8_externals ff8_externals;
extern struct common_externals common_externals;
extern struct driver_stats stats;

extern char popup_msg[];
extern uint32_t popup_ttl;
extern uint32_t popup_color;

extern struct game_mode modes[];
extern uint32_t num_modes;

extern uint32_t text_colors[];

extern uint32_t ff8;
extern uint32_t ff8_currentdisk;

extern uint32_t frame_counter;
extern double frame_rate;
extern int battle_frame_multiplier;
extern int common_frame_multiplier;

extern double speedhack_current;

extern bool xinput_connected;
extern bool simulate_OK_button;

extern char *get_current_field_name();
extern uint32_t noop();
