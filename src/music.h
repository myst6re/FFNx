/* 
 * FFNx - Complete OpenGL replacement of the Direct3D renderer used in 
 * the original ports of Final Fantasy VII and Final Fantasy VIII for the PC.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * music.h - music player definitions
 */

#pragma once

#include "types.h"

void music_init();
uint midi_init(uint unknown);
uint ff7_directsound_release();
void music_cleanup();
void play_midi(char* midi_name, uint midi, uint volume);
void ff7_play_midi(uint midi);
uint ff8_play_midi(uint midi, uint volume, uint u1, uint u2);
void pause_midi();
void restart_midi();
void stop_midi();
uint ff7_stop_midi();
uint ff8_stop_midi();
uint midi_status();
void set_direct_volume(uint volume);
uint ff7_set_direct_volume(DWORD volume);
uint ff8_set_direct_volume(int volume);
void set_midi_tempo(unsigned char tempo);
uint remember_playing_time();

bool needs_resume(uint old_mode, uint new_mode, char* old_midi, char* new_midi);
