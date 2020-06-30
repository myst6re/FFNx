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

#include <windows.h>
#include <DSound.h>
#include <mmreg.h>
#include <msacm.h>
extern "C" {
#include <vgmstream.h>
}

#include "patch.h"
#include "ff7.h"
#include "defs.h"

struct sfx_data {
	uint field_00;
	uint delay_related;
	uint field_08;
};

struct sfx_state {
	uint size;
	uint pos;
	uint flags;
	uint field_0C;
	uint delay;
	uint data_size;
	char* data; // recursive sfx_state ???
};

struct sound_obj {
	IDirectSoundBuffer* buffer;
	sfx_state* state;
	uint is_wav_song; // Init 0
	uint field_0C; // sound_obj.size_source_input / 4
	uint sound_buffer_size; // (sfx_state.field_0C / 0x5622 (+ 1 if sfx_state.field_0C != 0)) * sfx_state.field_0C
	uint exists; // In release
	uint paused;
	uint field_1C; // Init 0
	sfx_state* state_copy;
	uint field_24; // Init 0 size wat
	uint field_28; // Init 0 size wat
	uint buffer_write_pointer; // Init 0
	uint looped; // Init with state.flags
	uint field_34; // Init 0
	LPBYTE data_buffer_source;
	uint size_source_output;
	LPBYTE data_buffer_dest;
	LPHACMSTREAM phas;
	LPACMSTREAMHEADER pash;
	uint field_4C;
	uint field_50;
	uint field_54;
	uint field_58;
	uint field_5C;
	uint field_60;
	uint field_64;
	uint field_68;
	uint data_buffer_dest_size;
	uint field_70;
	uint field_74;
	uint field_78;
	uint field_7C;
	uint field_80;
	uint field_84;
	uint field_88;
	uint field_8C;
	uint field_90;
	uint field_94;
	uint field_98;
	uint field_9C; // Init 0 1 if state_copy.size < size_source_output
	uint data_buffer_dest_pos; // Init 0
	uint field_A4;
	uint file_pos;
	uint field_AC; // Init 0
};

struct AcmStream {
	MMRESULT(*size)(HACMSTREAM, DWORD, LPDWORD, DWORD);
	MMRESULT(*open)(LPHACMSTREAM, HACMDRIVER, LPWAVEFORMATEX, LPWAVEFORMATEX, LPWAVEFILTER, DWORD_PTR, DWORD_PTR, DWORD);
	MMRESULT(*convert)(HACMSTREAM, LPACMSTREAMHEADER, DWORD);
	MMRESULT(*prepareHeader)(HACMSTREAM, LPACMSTREAMHEADER, DWORD);
	MMRESULT(*unprepareHeader)(HACMSTREAM, LPACMSTREAMHEADER, DWORD);
	MMRESULT(*close)(HACMSTREAM, DWORD);
};

uint(*sound_error)(MMRESULT, const char*, int);
AcmStream acmStream;

void print_wave_format_ex(LPWAVEFORMATEX format)
{
	info("ACM Stream header: nAvgBytesPerSec %i nBlockAlign %i nChannels %i nSamplesPerSec %i wBitsPerSample %i wFormatTag %i\n", format->nAvgBytesPerSec, format->nBlockAlign, format->nChannels, format->nSamplesPerSec, format->wBitsPerSample, format->wFormatTag);
}

uint acm_stream_size(HACMSTREAM has, DWORD cbInput, LPDWORD outputBytes, DWORD fdwSize)
{
	trace("acm_stream_size %x %i %i\n", has, cbInput, fdwSize);
	return acmStream.size(has, cbInput, outputBytes, fdwSize);
}

uint acm_stream_open(LPHACMSTREAM phas, HACMDRIVER had, LPWAVEFORMATEX pwfxSrc, LPWAVEFORMATEX pwfxDst, LPWAVEFILTER pwfltr, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen)
{
	trace("acm_stream_open %x %i\n", phas, fdwOpen);
	print_wave_format_ex(pwfxSrc);
	print_wave_format_ex(pwfxDst);
	return acmStream.open(phas, had, pwfxSrc, pwfxDst, pwfltr, dwCallback, dwInstance, fdwOpen);
}

uint acm_stream_convert(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwConvert)
{
	trace("acm_stream_convert %x %i\n", has, fdwConvert);
	return acmStream.convert(has, pash, fdwConvert);
}

uint acm_stream_prepare_header(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwPrepare)
{
	trace("acm_stream_prepare_header %x %i\n", has, fdwPrepare);

	return acmStream.prepareHeader(has, pash, fdwPrepare);
}

uint acm_stream_unprepare_header(HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwUnprepare)
{
	trace("acm_stream_unprepare_header %x %i\n", has, fdwUnprepare);
	return acmStream.unprepareHeader(has, pash, fdwUnprepare);
}

uint acm_stream_close(HACMSTREAM has, DWORD fdwClose)
{
	trace("acm_stream_close %x %i\n", has, fdwClose);
	return acmStream.close(has, fdwClose);
}

void sfx_init2()
{
	acmStream.size = (MMRESULT(*)(HACMSTREAM, DWORD, LPDWORD, DWORD))0x7AF154;
	acmStream.open = (MMRESULT(*)(LPHACMSTREAM, HACMDRIVER, LPWAVEFORMATEX, LPWAVEFORMATEX, LPWAVEFILTER, DWORD_PTR, DWORD_PTR, DWORD))0x7AF15A;
	acmStream.convert = (MMRESULT(*)(HACMSTREAM, LPACMSTREAMHEADER, DWORD))0x7AF160;
	acmStream.prepareHeader = (MMRESULT(*)(HACMSTREAM, LPACMSTREAMHEADER, DWORD))0x7AF166;
	acmStream.unprepareHeader = (MMRESULT(*)(HACMSTREAM, LPACMSTREAMHEADER, DWORD))0x7AF16C;
	acmStream.close = (MMRESULT(*)(HACMSTREAM, DWORD))0x7AF172;
	sound_error = (uint(*)(MMRESULT, const char*, int))0x6E68F1;

	replace_call(0x6E7212 + 0x19, acm_stream_size);
	replace_call(0x6E7248 + 0x19, acm_stream_size);
	replace_call(0x6E727E + 0x35, acm_stream_open);
	replace_call(0x6E727E + 0x62, acm_stream_open);
	replace_call(0x6E72F6 + 0x50, acm_stream_convert);
	replace_call(0x6E72F6 + 0x2C, acm_stream_convert);
	replace_call(0x6E7358 + 0x51, acm_stream_prepare_header);
	replace_call(0x6E73BA + 0x17, acm_stream_unprepare_header);
	replace_call(0x6E73E1 + 0x13, acm_stream_close);
}
