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
extern "C" {
	#include <vgmstream.h>
}

#include "sfx.h"
#include "patch.h"
#include "ff7.h"

uint sfx_volumes[5];
ff7_field_sfx_state sfx_buffers[4];
uint real_volume;
int channel = 0;

void sound_stop(uint buffer)
{
	info("dsound_stop %i\n", buffer);

	uint* toto = (uint*)0xFFFFFFFF;
	*toto = 1;
}

void stop_dsound(IDirectSoundBuffer* buffer)
{
	info("dsound_stop %i\n", buffer);
	if (buffer) {
		buffer->Stop();
	}
}

uint start_dsound(IDirectSoundBuffer* buffer, uint flags)
{
	info("dsound_start %i\n", buffer);

	if (!buffer) {
		return 0;
	}

	HRESULT res = buffer->Play(0, 0, flags);

	if (DSERR_BUFFERLOST == res) {
		res = buffer->Restore();

		return -1;
	}

	return res == DS_OK;
}

void tifa_roulette(uint sound_id)
{
	((void(*)(uint))0x430D0A)(sound_id);
	replace_function(0x6E7080, sound_stop);
}

void sfx_play_on_channel(unsigned char panning, int sound_id, int channel)
{
	info("play sfx #%i (channel: %i, panning: %i)\n", sound_id, channel, panning);
}

void sfx_leviathan(unsigned char panning, int sound_id, unsigned char zz1)
{
	info("play leviathan #%i (zz1: %i, panning: %i)\n", sound_id, zz1, panning);

	if (zz1) {
		((void(*)(unsigned char, int, int))common_externals.play_sfx_on_channel)(panning, sound_id, channel + 1);
		channel = (channel + 1) % 3;
		//(void(*)(unsigned char, int, unsigned char))(0x5BBBAC)(panning, sound_id, zz1);
	}
}

void sfx_play_on_channels(int panning, int sound_id_channel_1, int sound_id_channel_2, int sound_id_channel_3, int sound_id_channel_4)
{
	info("play sfx #%i #%i #%i #%i (panning: %i)\n", sound_id_channel_1, sound_id_channel_2, sound_id_channel_3, sound_id_channel_4, panning);
}

void sfx_play_2(int sound_id, int panning, int volume, int frequency)
{
	info("play sfx #%i (panning: %i, volume: %i, frequency: %i)\n", sound_id, panning, volume, frequency);
}

void sfx_play_3(int sound_id)
{
	info("play sfx #%i\n", sound_id);
}

int sfx_play_battle_specific(IDirectSoundBuffer* buffer, uint flags)
{
	if (buffer == nullptr) {
		return 0;
	}

	unsigned char volume = 127 * (*common_externals.master_sfx_volume) / 100;
	info("sfx_play_battle_specific %i\n", volume);

	buffer->SetVolume(common_externals.dsound_volume_table[volume]);

	HRESULT res = buffer->Play(0, 0, flags);

	if (DSERR_BUFFERLOST == res) {
		res = buffer->Restore();

		return -1;
	}

	return res == DS_OK;
}

bool sfx_fill_buffer_external(uint sound_id, IDirectSoundBuffer** sound_buffer)
{
	char filename[MAX_PATH];
	char* sound_dir = (char*)0xDC3398;

	sprintf(filename, "%s/sound-%d.%s", sound_dir, sound_id, external_music_ext);

	info("sfx_fill_buffer_external %s\n", filename);

	if (_access(filename, 0) != 0) {
		return false;
	}

	trace("sfx_fill_buffer_external 2 %s\n", filename);

	VGMSTREAM *stream = init_vgmstream(filename);

	if (stream == nullptr) {
		return false;
	}

	trace("sfx_fill_buffer_external %i %i\n", stream->num_samples, stream->channels);

	sample_t* sample_buffer = (sample_t*)driver_malloc(stream->num_samples * sizeof(sample_t) * stream->channels);

	if (sample_buffer == nullptr) {
		return false;
	}

	render_vgmstream(sample_buffer, stream->num_samples, stream);

	close_vgmstream(stream);

	DSBUFFERDESC1 sbdesc = DSBUFFERDESC1();
	WAVEFORMATEX sound_format;

	sound_format.cbSize = 0;
	sound_format.wBitsPerSample = sizeof(sample_t) * 8;
	sound_format.nChannels = stream->channels;
	sound_format.nSamplesPerSec = stream->sample_rate;
	sound_format.nBlockAlign = sound_format.nChannels * sound_format.wBitsPerSample / 8;
	sound_format.nAvgBytesPerSec = sound_format.nSamplesPerSec * sound_format.nBlockAlign;
	sound_format.wFormatTag = WAVE_FORMAT_PCM;

	sbdesc.dwSize = sizeof(sbdesc);
	sbdesc.lpwfxFormat = &sound_format;
	sbdesc.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY
		| DSBCAPS_CTRLPAN | DSBCAPS_GETCURRENTPOSITION2
		| DSBCAPS_TRUEPLAYPOSITION | DSBCAPS_GLOBALFOCUS;
	sbdesc.dwReserved = 0;
	sbdesc.dwBufferBytes = stream->num_samples * sound_format.nBlockAlign;

	if (!*common_externals.directsound)
	{
		error("SFX No directsound device\n");
		driver_free(sample_buffer);

		return false;
	}

	if ((*common_externals.directsound)->CreateSoundBuffer((LPCDSBUFFERDESC)&sbdesc, sound_buffer, 0))
	{
		error("SFX couldn't create sound buffer (%i, %i)\n", sound_format.nChannels, sound_format.nSamplesPerSec);
		driver_free(sample_buffer);

		return false;
	}

	LPVOID ptr1, ptr2;
	DWORD bytes1, bytes2;

	if ((*sound_buffer)->Lock(0, sbdesc.dwBufferBytes, &ptr1, &bytes1, &ptr2, &bytes2, 0)) {
		error("SFX couldn't lock sound buffer\n");
		driver_free(sample_buffer);
		return false;
	}

	memcpy(ptr1, sample_buffer, bytes1);
	memcpy(ptr2, (char*)sample_buffer + bytes1, bytes2);

	if ((*sound_buffer)->Unlock(ptr1, bytes1, ptr2, bytes2)) {
		error("SFX couldn't unlock sound buffer\n");
		driver_free(sample_buffer);
		return false;
	}

	driver_free(sample_buffer);

	return true;
}

bool sfx_fill_buffer_patial_external(uint sound_id, void* sound_structure)
{
	return false; // TODO: play sound_id
}

bool sfx_fill_buffer(uint sound_id, IDirectSoundBuffer** buffer)
{
	trace("sfx_fill_buffer_1_A %i %X\n", sound_id, buffer);

	sound_id -= 1;
	IDirectSoundBuffer** dsound_buffers = (IDirectSoundBuffer**)0xDBD068;
	uint static_sound_loaded = *((uint*)dsound_buffers - 2);

	trace("sfx_fill_buffer_1_B %X %i %X\n", *dsound_buffers, static_sound_loaded, (uint*)dsound_buffers - 2);

	if (sound_id < 0 || sound_id > 750 || !static_sound_loaded) {
		return false;
	}

	trace("sfx_fill_buffer_1_C %X %X\n", &dsound_buffers[sound_id], dsound_buffers[sound_id]);

	if (dsound_buffers[sound_id] != nullptr) {
		return true;
	}

	if (buffer == nullptr) {
		buffer = dsound_buffers + sound_id;
	}

	uint* sound_structure = (uint*)0xDBDDC4 + 7 * sound_id;
	// { uint size, uint pos, ... unknown }, sizeof = 0x1C
	uint pos = sound_structure[1];
	trace("sfx_fill_buffer_1_D %X %i\n", buffer, pos);

	for (int i = 0; i < 21; ++i) {
		trace("sound_structure %X: %i\n", sound_structure + i, sound_structure[i]);
	}

	if (sound_structure[0] == 0) {
		return false;
	}

	if (sfx_fill_buffer_external(sound_id, buffer)) {
		return true;
	}

	uint fd_audiodat = *(uint*)0xDC338C;

	// seek (SEEK_SET)
	((void(*)(int, long))0x68289A)(fd_audiodat, pos);
	// open and create sound buffer
	*buffer = ((IDirectSoundBuffer * (*)(uint*, int))0x6E26A2)(sound_structure, fd_audiodat);

	return *buffer != nullptr;
}

bool sfx_fill_buffer_partial(uint sound_id, void* sound_structure)
{
	info("sfx_fill_buffer_2 %i %i\n", sound_id, sound_structure);

	if (sfx_fill_buffer_patial_external(sound_id - 1, sound_structure)) {
		return true;
	}

	return ((uint(*)(uint, void*))0x6E3643)(sound_id, sound_structure);
}

void sfx_init()
{
	// Add Global Focus flag to DirectSound Secondary Buffers
	patch_code_byte(common_externals.directsound_buffer_flags_1 + 0x4, 0x80); // DSBCAPS_GLOBALFOCUS & 0x0000FF00

	// External SFX
	if (!ff8) {
		replace_function(0x6E2573, sfx_fill_buffer);
		replace_call(0x6E1E86 + 0x105, sfx_fill_buffer_partial);
	}

	// SFX Patches
	if (!ff8) {
		// On volume change in main menu initialization
		replace_call(ff7_externals.menu_start + 0x17, sfx_menu_force_channel_5_volume);
		// On SFX volume change in config menu
		replace_call(ff7_externals.menu_sound_slider_loop + ff7_externals.call_menu_sound_slider_loop_sfx_down, sfx_menu_play_sound_down);
		replace_call(ff7_externals.menu_sound_slider_loop + ff7_externals.call_menu_sound_slider_loop_sfx_up, sfx_menu_play_sound_up);
		// Fix escape sound not played more than once
		replace_function(ff7_externals.battle_clear_sound_flags, sfx_clear_sound_locks);
		// On stop sound in battle swirl
		replace_call(ff7_externals.swirl_sound_effect + 0x26, sfx_operation_battle_swirl_stop_sound);
		// On resume music after a battle
		replace_call(ff7_externals.field_initialize_variables + 0xEB, sfx_operation_resume_music);
		// Fix volume on specific SFX
		replace_call(0x6E580F + 0xA2, sfx_play_battle_specific);
		replace_call(0x6E580F + 0xF2, sfx_play_battle_specific);

		//replace_function(common_externals.play_sfx_on_channel, sfx_play_on_channel);
		//replace_call(0x5BBA92 + 0x93, sfx_leviathan);
		//replace_function(0x6E1E86, sfx_play_on_channels);
		//replace_function(0x6E55E9, sfx_play_2);
		//replace_function(0x6E580F, sfx_play_3);
		//replace_function(0x6E7080, stop_dsound);
		//replace_function(0x6E7015, start_dsound);

		//replace_call(0x7640AA + 0x56C, tifa_roulette);

		// Leviathan fix
		patch_code_byte(0x5B1B7F + 1, 0x29);
		patch_code_byte(0x5B1B8D + 1, 0x2A);

		/* Set sound volume on channel changes
		 * When this sub is called, it set two fields of sfx_state,
		 * but with the wrong value (computed with sfx_master_volume)
		 */

		// Replace a useless "volume & 0xFF" to "real_volume <- volume; nop"
		patch_code_byte(uint(common_externals.set_sfx_volume_on_channel) + 0x48, 0xA3); // mov
		patch_code_uint(uint(common_externals.set_sfx_volume_on_channel) + 0x48 + 1, uint(&real_volume));
		patch_code_byte(uint(common_externals.set_sfx_volume_on_channel) + 0x48 + 5, 0x90); // nop
		// Use a field of sfx_state to flag the current channel
		patch_code_uint(uint(common_externals.set_sfx_volume_on_channel) + 0x70, 0xFFFFFFFF);
		// Replace log call to fix sfx_state volume values
		replace_call(uint(common_externals.set_sfx_volume_on_channel) + 0x183, sfx_fix_volume_values);
	}
}

bool sfx_buffer_is_looped(IDirectSoundBuffer* buffer)
{
	if (buffer == nullptr) {
		return false;
	}

	DWORD status;
	buffer->GetStatus(&status);

	if (status & (DSBSTATUS_LOOPING | DSBSTATUS_PLAYING)) {
		return true;
	}

	return false;
}

uint sfx_operation_battle_swirl_stop_sound(uint type, uint param1, uint param2, uint param3, uint param4, uint param5)
{
	if (trace_all || trace_music) info("Battle swirl stop sound\n");

	ff7_field_sfx_state* sfx_state = ff7_externals.sound_states;

	for (int i = 0; i < 4; ++i) {
		sfx_buffers[i] = ff7_field_sfx_state();
		sfx_buffers[i].buffer1 = nullptr;
		sfx_buffers[i].buffer2 = nullptr;
		// Save sfx state for looped sounds in channel 1 -> 4 (not channel 5)
		if (sfx_buffer_is_looped(sfx_state[i].buffer1) || sfx_buffer_is_looped(sfx_state[i].buffer2)) {
			memcpy(&sfx_buffers[i], &sfx_state[i], sizeof(ff7_field_sfx_state));
		}
	}

	return ((uint(*)(uint, uint, uint, uint, uint, uint))ff7_externals.sound_operation)(type, param1, param2, param3, param4, param5);
}

uint sfx_operation_resume_music(uint type, uint param1, uint param2, uint param3, uint param4, uint param5)
{
	if (trace_all || trace_music) info("Field resume music after battle\n");

	for (int i = 0; i < 4; ++i) {
		if (sfx_buffers[i].buffer1 != nullptr || sfx_buffers[i].buffer2 != nullptr) {
			uint pan;
			if (sfx_buffers[i].buffer1 != nullptr) {
				pan = sfx_buffers[i].pan1;
			}
			else {
				pan = sfx_buffers[i].pan2;
			}

			((uint(*)(uint, uint, uint))common_externals.play_sfx_on_channel)(pan, sfx_buffers[i].sound_id, i + 1);

			sfx_buffers[i] = ff7_field_sfx_state();
			sfx_buffers[i].buffer1 = nullptr;
			sfx_buffers[i].buffer2 = nullptr;
		}
	}

	return ((uint(*)(uint, uint, uint, uint, uint, uint))ff7_externals.sound_operation)(type, param1, param2, param3, param4, param5);
}

void sfx_remember_volumes()
{
	if (trace_all || trace_music) info("Remember SFX volumes (master: %i)\n", *common_externals.master_sfx_volume);

	ff7_field_sfx_state* sfx_state = ff7_externals.sound_states;

	for (int i = 0; i < 5; ++i) {
		sfx_volumes[i] = sfx_state[i].buffer1 != nullptr ? sfx_state[i].volume1 : sfx_state[i].volume2;

		if (sfx_volumes[i] > 127) {
			sfx_volumes[i] = 127;
		}

		if (trace_all || trace_music) info("SFX volume channel #%i: %i\n", i, sfx_volumes[i]);
	}
}

void sfx_menu_force_channel_5_volume(uint volume, uint channel)
{
	if (trace_all || trace_music) info("sfx_menu_force_channel_5_volume %d\n", volume);
	// Original call (set channel 5 volume to maximum)
	common_externals.set_sfx_volume_on_channel(volume, channel);
	// Added by FFNx
	sfx_remember_volumes();
}

void sfx_update_volume(int modifier)
{
	if (trace_all || trace_music) info("Update SFX volumes %d\n", modifier);

	// Set master sfx volume
	BYTE** sfx_tmp_volume = (BYTE**)(ff7_externals.menu_sound_slider_loop + ff7_externals.call_menu_sound_slider_loop_sfx_down + 0xA);

	*common_externals.master_sfx_volume = **sfx_tmp_volume + modifier;

	// Update sfx volume in real-time for all channel
	for (int channel = 1; channel <= 5; ++channel) {
		common_externals.set_sfx_volume_on_channel(sfx_volumes[channel - 1], channel);

		if (trace_all || trace_music) info("Set SFX volume for channel #%i: %i\n", channel, sfx_volumes[channel - 1]);
	}
}

void sfx_menu_play_sound_down(uint id)
{
	// Added by FFNx
	sfx_update_volume(-1);
	// Original call (curor sound)
	common_externals.play_sfx(id);
}

void sfx_menu_play_sound_up(uint id)
{
	// Added by FFNx
	sfx_update_volume(1);
	// Original call (curor sound)
	common_externals.play_sfx(id);
}

void sfx_clear_sound_locks()
{
	uint** flags = (uint**)(ff7_externals.battle_clear_sound_flags + 5);
	// The last uint wasn't reset by the original sub
	memset((void*)*flags, 0, 5 * sizeof(uint));
}

void sfx_fix_volume_values(char* log)
{
	if (trace_all || trace_music) info("%s", log); // FF7 default log

	ff7_field_sfx_state* sfx_state = ff7_externals.sound_states;

	for (int i = 0; i < 5; ++i) {
		if (sfx_state[i].u1 == 0xFFFFFFFF) {
			if (trace_all || trace_music) info("SFX fix volume channel #%i: %i\n", i + 1, real_volume);

			sfx_state[i].volume1 = real_volume;
			sfx_state[i].volume2 = real_volume;
			sfx_state[i].u1 = 0; // Back to the correct value
		}
	}
}
