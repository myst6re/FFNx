/*
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU Lesser General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU Lesser General Public License for more details.
 *
 *	You should have received a copy of the GNU Lesser General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "music.h"

AbstractOutPlugin* out = nullptr;
InPluginWithFailback* in = nullptr;

uint winamp_current_id = 0;
char* winamp_current_midi = nullptr;
bool winamp_song_ended = true;
bool winamp_song_paused = false;

uint winamp_paused_midi_id = 0;
uint winamp_current_mode = uint(-1);

void winamp_load_song(char* midi, uint id, uint volume)
{
	char filename[MAX_PATH];

	if (!in || !out)
	{
		return;
	}

	if (trace_all || trace_music) trace("load song %s %i\n", midi, id);
	
	uint winamp_previous_paused_midi_id = winamp_paused_midi_id;
	uint winamp_previous_mode = winamp_current_mode;
	char* winamp_previous_midi = winamp_current_midi;
	uint mode = getmode_cached()->driver_mode;

	if (!id)
	{
		in->stop();
		winamp_song_ended = true;
		winamp_current_id = 0;
		return;
	}
	
	sprintf(filename, "%s/%s/%s.%s", basedir, external_music_path, midi, external_music_ext);

	int play_err = 0;
	
	if (!ff8 && winamp_current_id && needs_resume(mode, winamp_previous_mode, midi, winamp_previous_midi)) {
		winamp_remember_playing_time();
	}

	winamp_song_ended = false;
	winamp_current_id = id;
	winamp_current_midi = midi;
	winamp_current_mode = mode;

	out->setVolume(volume * 255 / 127);

	if (winamp_previous_paused_midi_id == id
			&& needs_resume(winamp_previous_mode, mode, winamp_previous_midi, midi)) {
		info("Resuming midi to previous time\n");
		play_err = in->resume(filename);
		winamp_paused_midi_id = 0;
	}
	else {
		in->stop();
		play_err = in->play(filename);
	}
	
	if (-1 == play_err) {
		error("couldn't play music (file not found)\n");
	} else if (0 != play_err) {
		error("couldn't play music (%i)\n", play_err);
	}
}

void winamp_music_init()
{
	if (nullptr != out) {
		delete out;
		out = nullptr;
	}

	char* out_type = "FFNx out implementation",
		* in_type = "FFNx in implementation";
	
	if (nullptr != winamp_out_plugin) {
		WinampOutPlugin* winamp_out = new WinampOutPlugin();
		if (winamp_out->open(winamp_out_plugin)) {
			out = winamp_out;
			out_type = winamp_out_plugin;

			// Force volume for MM (out_wave fix)
			for (int i = 0; i < waveInGetNumDevs(); ++i) {
				waveOutSetVolume(HWAVEOUT(i), 0xFFFFFFFF);
			}
		}
		else {
			error("couldn't load %s, please verify 'winamp_out_plugin' or comment it\n", winamp_out_plugin);
			delete winamp_out;
		}
	}

	if (nullptr == out) {
		out = new CustomOutPlugin();
	}

	WinampInPlugin* winamp_in = nullptr;

	if (nullptr != winamp_in_plugin) {
		winamp_in = new WinampInPlugin(out);
		if (winamp_in->open(winamp_in_plugin)) {
			in_type = winamp_in_plugin;
		}
		else {
			error("couldn't load %s, please verify 'winamp_in_plugin' or comment it\n", winamp_in_plugin);
			delete winamp_in;
			winamp_in = nullptr;
		}
	}
	
	if (nullptr == winamp_in) {
		in = new InPluginWithFailback(out, new VgmstreamInPlugin(out));
	}
	else {
		in = new InPluginWithFailback(out, winamp_in, new VgmstreamInPlugin(out));
	}
	
	info("Winamp music plugin loaded using %s and %s\n", in_type, out_type);
}

// start playing some music, <midi> is the name of the MIDI file without the .mid extension
void winamp_play_music(char *midi, uint id, uint volume)
{
	if (trace_all || trace_music) trace("[%s] play music: %s:%i (current=%i, ended=%i)\n", getmode_cached()->name, midi, id, winamp_current_id, winamp_song_ended);
	
	if (id != winamp_current_id || winamp_song_ended)
	{
		winamp_load_song(midi, id, volume);
	}
}

void winamp_stop_music()
{
	if (trace_all || trace_music) trace("[%s] stop music\n", getmode_cached()->name);

	if (in) {
		in->stop();
	}
	
	winamp_song_ended = true;
	winamp_current_id = 0;
}

void winamp_pause_music()
{
	if (trace_all || trace_music) trace("Pause music\n");
	
	if (in) {
		in->pause();
	}
	winamp_song_paused = true;
}

void winamp_resume_music()
{
	if (trace_all || trace_music) trace("Resume music\n");

	if (in) {
		in->unPause();
	}
	winamp_song_paused = false;
}

// return true if music is playing, false if it isn't
// it's important for some field scripts that this function returns true atleast once when a song has been requested
// 
// even if there's nothing to play because of errors/missing files you cannot return false every time
bool winamp_music_status()
{
	uint last_status = 0;
	uint status;

	if (!in)
	{
		last_status = !last_status;
		return !last_status;
	}
	else {
		status = out->getOutputTime() < in->getLength();
	}

	last_status = status;
	return status;
}

void winamp_set_direct_volume(int volume)
{
	if (trace_all || trace_music) trace("Set direct volume %i\n", volume);

	if (out)
	{
		out->setVolume(volume);
	}
}

void winamp_set_music_tempo(unsigned char tempo)
{
	if (trace_all || trace_music) trace("set music tempo: %i\n", int(tempo));

	if (out) {
		out->setTempo(tempo);
	}
}

void winamp_remember_playing_time()
{
	if (in && winamp_current_id) {
		info("Saving midi time for later use\n");
		winamp_paused_midi_id = winamp_current_id;
		in->duplicate();
	}
}

void winamp_music_cleanup()
{
	if (trace_all || trace_music) trace("music cleanup\n");
	
	winamp_stop_music();

	if (in) {
		delete in->getPlugin1();
		if (in->getPlugin2()) {
			delete in->getPlugin2();
		}
		delete in;
		in = nullptr;
	}

	if (out) {
		delete out;
		out = nullptr;
	}
}
