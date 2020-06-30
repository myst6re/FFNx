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

#include "sfx.h"
#include "patch.h"
#include "ff7.h"
#include "ff7/defs.h"

uint sfx_volumes[5];
ff7_field_sfx_state sfx_buffers[4];
uint real_volume;

void sfx_init()
{
	// Add Global Focus flag to DirectSound Secondary Buffers
	patch_code_byte(common_externals.directsound_buffer_flags_1 + 0x4, 0x80); // DSBCAPS_GLOBALFOCUS & 0x0000FF00

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
		replace_call(ff7_externals.sfx_play_summon + 0xA2, sfx_play_battle_specific);
		replace_call(ff7_externals.sfx_play_summon + 0xF2, sfx_play_battle_specific);

		// Leviathan fix
		patch_code_byte(ff7_externals.battle_summon_leviathan_loop + 0x3FA + 1, 0x29);
		patch_code_byte(ff7_externals.battle_summon_leviathan_loop + 0x408 + 1, 0x2A);
		// Omnislash fix
		replace_call(ff7_externals.battle_limit_omnislash_loop + 0x5A, sfx_fix_omnislash_sound_loading);

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

int sfx_play_battle_specific(IDirectSoundBuffer* buffer, uint flags)
{
	if (buffer == nullptr) {
		return 0;
	}

	// Added by FFNx: set buffer volume according to master_sfx_volume
	unsigned char volume = 127 * (*common_externals.master_sfx_volume) / 100;

	buffer->SetVolume(common_externals.dsound_volume_table[volume]);

	// Original behavior
	HRESULT res = buffer->Play(0, 0, flags);

	if (DSERR_BUFFERLOST == res) {
		res = buffer->Restore();

		return -1;
	}

	return res == DS_OK;
}

uint sfx_fix_omnislash_sound_loading(int sound_id, int dsound_buffer)
{
	// Added by FFNx: Load sound 0x188
	((uint(*)(int, int))ff7_externals.sfx_fill_buffer_from_audio_dat)(0x188, dsound_buffer);

	// Original call (load sound 0x285)
	return ((uint(*)(int, int))ff7_externals.sfx_fill_buffer_from_audio_dat)(sound_id, dsound_buffer);
}
