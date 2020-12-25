/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Julian Xhokaxhiu                                   //
//    Copyright (C) 2020 myst6re                                            //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
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

#include "audio.h"

#include "log.h"
#include "gamehacks.h"

NxAudioEngine nxAudioEngine;

// PRIVATE

void NxAudioEngine::loadConfig()
{
	char _fullpath[MAX_PATH];

	for (int idx = NxAudioEngineLayer::NXAUDIOENGINE_SFX; idx != NxAudioEngineLayer::NXAUDIOENGINE_VOICE; idx++)
	{
		NxAudioEngineLayer type = NxAudioEngineLayer(idx);

		switch (type)
		{
		case NxAudioEngineLayer::NXAUDIOENGINE_SFX:
			sprintf(_fullpath, "%s/%s/config.toml", basedir, external_sfx_path.c_str());
			break;
		case NxAudioEngineLayer::NXAUDIOENGINE_MUSIC:
			sprintf(_fullpath, "%s/%s/config.toml", basedir, external_music_path.c_str());
			break;
		case NxAudioEngineLayer::NXAUDIOENGINE_VOICE:
			sprintf(_fullpath, "%s/%s/config.toml", basedir, external_voice_path.c_str());
			break;
		}

		try
		{
			nxAudioEngineConfig[type] = toml::parse_file(_fullpath);
		}
		catch (const toml::parse_error &err)
		{
			nxAudioEngineConfig[type] = toml::parse("");
		}
	}
}

template <class T>
bool NxAudioEngine::getFilenameFullPath(char *_out, T _key, NxAudioEngineLayer _type)
{
	std::vector<std::string> extensions;

	switch(_type)
	{
		case NxAudioEngineLayer::NXAUDIOENGINE_SFX:
			extensions = external_sfx_ext;
			break;
		case NxAudioEngineLayer::NXAUDIOENGINE_MUSIC:
			extensions = external_music_ext;
			break;
		case NxAudioEngineLayer::NXAUDIOENGINE_VOICE:
			extensions = external_voice_ext;
			break;
	}

	for (const std::string &extension: extensions) {
		switch (_type)
		{
		case NxAudioEngineLayer::NXAUDIOENGINE_SFX:
			sprintf(_out, "%s/%s/%d.%s", basedir, external_sfx_path.c_str(), _key, extension.c_str());
			break;
		case NxAudioEngineLayer::NXAUDIOENGINE_MUSIC:
			sprintf(_out, "%s/%s/%s.%s", basedir, external_music_path.c_str(), _key, extension.c_str());
			break;
		case NxAudioEngineLayer::NXAUDIOENGINE_VOICE:
			sprintf(_out, "%s/%s/%s.%s", basedir, external_voice_path.c_str(), _key, extension.c_str());
			break;
		}

		if (fileExists(_out)) {
			return true;
		}
	}

	return false;
}

bool NxAudioEngine::fileExists(const char* filename)
{
	struct stat dummy;

	bool ret = (stat(filename, &dummy) == 0);

	if (!ret && (trace_all || trace_music || trace_sfx || trace_voice)) warning("NxAudioEngine::%s: Could not find file %s\n", __func__, filename);

	return ret;
}

// PUBLIC

bool NxAudioEngine::init()
{
	if (_engine.init() == 0)
	{
		_engineInitialized = true;

		loadConfig();

		if (!he_bios_path.empty()) {
			if (!Psf::initialize_psx_core(he_bios_path.c_str())) {
				error("NxAudioEngine::%s couldn't load %s, please verify 'he_bios_path' or comment it\n", __func__, he_bios_path.c_str());
			}
			else {
				_openpsf_loaded = true;
				info("NxAudioEngine::%s OpenPSF music plugin loaded using %s\n", __func__, he_bios_path.c_str());
			}
		}

		_sfxVolumePerChannels.resize(10, 1.0f);
		_sfxTempoPerChannels.resize(10, 1.0f);
		_sfxChannelsHandle.resize(10, NXAUDIOENGINE_INVALID_HANDLE);
		_sfxStreams.resize(10000, nullptr);

		while (!_sfxStack.empty())
		{
			loadSFX(_sfxStack.top());
			_sfxStack.pop();
		}

		return true;
	}

	return false;
}

void NxAudioEngine::flush()
{
	_engine.stopAll();

	_musicStack.empty();
	_musics[0].handle = NXAUDIOENGINE_INVALID_HANDLE;
	_musics[1].handle = NXAUDIOENGINE_INVALID_HANDLE;

	_voiceHandle = NXAUDIOENGINE_INVALID_HANDLE;
}

void NxAudioEngine::cleanup()
{
	_engine.deinit();
}

// SFX
bool NxAudioEngine::canPlaySFX(int id)
{
	char filename[MAX_PATH];

	return getFilenameFullPath<int>(filename, id, NxAudioEngineLayer::NXAUDIOENGINE_SFX);
}

void NxAudioEngine::loadSFX(int id)
{
	int _curId = id - 1;

	if (_engineInitialized)
	{
		if (_sfxStreams[_curId] == nullptr)
		{
			char filename[MAX_PATH];

			bool exists = getFilenameFullPath<int>(filename, id, NxAudioEngineLayer::NXAUDIOENGINE_SFX);

			if (trace_all || trace_sfx) trace("NxAudioEngine::%s: %s\n", __func__, filename);

			if (exists)
			{
				SoLoud::Wav* sfx = new SoLoud::Wav();

				sfx->load(filename);

				_sfxStreams[_curId] = sfx;
			}
		}
	}
	else
		_sfxStack.push(id);
}

void NxAudioEngine::unloadSFX(int id)
{
	int _curId = id - 1;

	if (_sfxStreams[_curId] != nullptr)
	{
		delete _sfxStreams[_curId];

		_sfxStreams[_curId] = nullptr;
	}
}

void NxAudioEngine::playSFX(int id, int channel, float panning)
{
	int _curId = id - 1;

	std::string _id = std::to_string(id);
	auto node = nxAudioEngineConfig[NxAudioEngineLayer::NXAUDIOENGINE_SFX][_id];
	if (node)
	{
		// Shuffle SFX playback, if any entry found for the current id
		toml::array *shuffleIds = node["shuffle"].as_array();
		if (!shuffleIds->empty() && shuffleIds->is_homogeneous(toml::node_type::integer))
		{
			auto _newId = shuffleIds->get(getRandomInt(0, shuffleIds->size() - 1));

			_curId = _newId->value_or(id) - 1;
		}
	}

	if (trace_all || trace_sfx) trace("NxAudioEngine::%s: id=%d,channel=%d,panning:%f\n", __func__, _curId + 1, channel, panning);

	// Try to load the ID if it's new to the audio engine
	if (_sfxStreams[_curId] == nullptr) loadSFX(_curId + 1);

	if (_sfxStreams[_curId] != nullptr)
	{
		SoLoud::handle _handle = _engine.play(
			*_sfxStreams[_curId],
			_sfxVolumePerChannels[channel - 1],
			panning
		);

		_sfxChannelsHandle[channel - 1] = _handle;

		_engine.setRelativePlaySpeed(_handle, _sfxTempoPerChannels[channel - 1]);
	}
}

void NxAudioEngine::pauseSFX()
{
	for (auto _handle : _sfxChannelsHandle) _engine.setPause(_handle, true);
}

void NxAudioEngine::resumeSFX()
{
	for (auto _handle : _sfxChannelsHandle) _engine.setPause(_handle, false);
}

void NxAudioEngine::setSFXVolume(float volume, int channel)
{
	_sfxVolumePerChannels[channel - 1] = volume;
}

void NxAudioEngine::setSFXSpeed(float speed, int channel)
{
	_sfxTempoPerChannels[channel - 1] = speed;
}

// Music
bool NxAudioEngine::canPlayMusic(const char* name)
{
	char filename[MAX_PATH];

	return getFilenameFullPath<const char*>(filename, name, NxAudioEngineLayer::NXAUDIOENGINE_MUSIC);
}

SoLoud::AudioSource* NxAudioEngine::loadMusic(const char* name, bool isFullPath)
{
	SoLoud::AudioSource* music = nullptr;
	char filename[MAX_PATH];
	bool exists = false;

	if (isFullPath)
	{
		exists = fileExists(name);
		strcpy(filename, name);
	}

	if (!exists)
	{
		exists = getFilenameFullPath<const char*>(filename, name, NxAudioEngineLayer::NXAUDIOENGINE_MUSIC);
	}

	if (exists)
	{
		if (trace_all || trace_music) trace("NxAudioEngine::%s: %s\n", __func__, filename);

		if (_openpsf_loaded && SoLoud::OpenPsf::is_our_path(filename)) {
			SoLoud::OpenPsf* openpsf = new SoLoud::OpenPsf();
			music = openpsf;

			if (openpsf->load(filename) != SoLoud::SO_NO_ERROR) {
				error("NxAudioEngine::%s: Cannot load %s with openpsf\n", __func__, filename);
				delete openpsf;
				music = nullptr;
			}
		}

		if (music == nullptr) {
			SoLoud::VGMStream* vgmstream = new SoLoud::VGMStream();
			music = vgmstream;
			if (vgmstream->load(filename) != SoLoud::SO_NO_ERROR) {
				error("NxAudioEngine::%s: Cannot load %s with vgmstream\n", __func__, filename);
			}
		}
	}

	return music;
}

void NxAudioEngine::overloadPlayArgumentsFromConfig(char* name, uint32_t* id, PlayOptions* playOptions)
{
	toml::table config = nxAudioEngineConfig[NXAUDIOENGINE_MUSIC];
	std::optional<SoLoud::time> offset_seconds_opt = config[name]["offset_seconds"].value<SoLoud::time>();
	std::optional<std::string> no_intro_track_opt = config[name]["no_intro_track"].value<std::string>();
	std::optional<SoLoud::time> intro_seconds_opt = config[name]["intro_seconds"].value<SoLoud::time>();

	if (playOptions->noIntro) {
		if (no_intro_track_opt.has_value()) {
			std::string no_intro_track = *no_intro_track_opt;
			if (trace_all || trace_music) info("%s: replaced by no intro track %s\n", __func__, no_intro_track.c_str());

			if (!no_intro_track.empty()) {
				memcpy(name, no_intro_track.c_str(), no_intro_track.size());
				name[no_intro_track.size()] = '\0';
			}
		}
		else if (intro_seconds_opt.has_value()) {
			playOptions->offsetSeconds = *intro_seconds_opt;
		}
		else {
			info("%s: cannot play no intro track, please configure it in %s/config.toml\n", __func__, external_music_path.c_str());
		}
	} else if (offset_seconds_opt.has_value()) {
		playOptions->offsetSeconds = *offset_seconds_opt;
	}

	// Name to lower case
	for (int i = 0; name[i]; i++) {
		name[i] = tolower(name[i]);
	}
	// Shuffle Music playback, if any entry found for the current music name
	toml::array* shuffleNames = config[name]["shuffle"].as_array();
	if (shuffleNames && !shuffleNames->empty() && shuffleNames->is_homogeneous(toml::node_type::string)) {
		std::optional<std::string> _newName = shuffleNames->get(getRandomInt(0, shuffleNames->size() - 1))->value<std::string>();
		if (_newName.has_value()) {
			memcpy(name, (*_newName).c_str(), (*_newName).size());
			name[(*_newName).size()] = '\0';

			if (trace_all || trace_music) info("%s: replaced by shuffle with %s\n", __func__, (*_newName).c_str());
		}
	}
}

bool NxAudioEngine::playMusic(char* name, uint32_t id, int channel, PlayOptions& playOptions)
{
	if (trace_all || trace_music) trace("NxAudioEngine::%s: %s (%d) on channel #%d\n", __func__, name, id, channel);

	if (!playOptions.useNameAsFullPath) {
		overloadPlayArgumentsFromConfig(name, &id, &playOptions);
	}

	if (!_musicStack.empty() && _musicStack.top().id == id) {
		resumeMusic(channel, playOptions.fadetime == 0.0 ? 1.0 : playOptions.fadetime, true, playOptions.flags); // Slight fade
		return true;
	}

	NxAudioEngineMusic& music = _musics[channel];

	if (_engine.isValidVoiceHandle(music.handle)) {
		if (music.id == id) {
			if (trace_all || trace_music) trace("NxAudioEngine::%s: %s is already playing on channel %d\n", __func__, name, channel);
			return false; // Already playing
		}

		if (!(playOptions.flags & PlayFlagsDoNotPause) && music.isResumable) {
			pauseMusic(channel, playOptions.fadetime, true);
		}
		else {
			stopMusic(channel, playOptions.fadetime);
		}
	}

	SoLoud::AudioSource* audioSource = loadMusic(name, playOptions.useNameAsFullPath);

	if (audioSource != nullptr) {
		if (playOptions.targetVolume >= 0.0f) {
			music.wantedMusicVolume = playOptions.targetVolume;
		}
		const float initialVolume = playOptions.fadetime > 0.0 ? 0.0f : music.wantedMusicVolume * _musicMasterVolume;
		music.handle = _engine.playBackground(*audioSource, initialVolume, playOptions.offsetSeconds > 0);
		music.id = id;
		music.isResumable = playOptions.flags & PlayFlagsIsResumable;

		if (playOptions.offsetSeconds > 0) {
			if (trace_all || trace_music) info("%s: seek to time %fs\n", __func__, playOptions.offsetSeconds);
			_engine.seek(music.handle, playOptions.offsetSeconds);
			resumeMusic(channel, playOptions.fadetime == 0.0 ? 1.0 : playOptions.fadetime); // Slight fade
		}
		else if (playOptions.fadetime > 0.0) {
			setMusicVolume(music.wantedMusicVolume, channel, playOptions.fadetime);
		}

		return true;
	}

	return false;
}

void NxAudioEngine::playSynchronizedMusics(const std::vector<std::string>& names, uint32_t id, PlayOptions& playOptions)
{
	if (_musics[0].id == id) {
		if (trace_all || trace_music) trace("NxAudioEngine::%s: id %d is already playing\n", __func__, id);
		return; // Already playing
	}

	if (trace_all || trace_music) trace("NxAudioEngine::%s: id %d\n", __func__, id);

	stopMusic(0, playOptions.fadetime);
	stopMusic(1, playOptions.fadetime);

	SoLoud::handle groupHandle = _engine.createVoiceGroup();

	if (groupHandle == 0) {
		error("NxAudioEngine::%s: cannot allocate voice group\n", __func__);
		return;
	}

	for (const std::string &name: names) {
		SoLoud::AudioSource* audioSource = loadMusic(name.c_str());
		if (audioSource != nullptr) {
			SoLoud::handle musicHandle = _engine.playBackground(*audioSource, -1.0f, true);
			_engine.addVoiceToGroup(groupHandle, musicHandle);
		}
	}

	if (!_engine.isVoiceGroupEmpty(groupHandle)) {
		_musics[0].handle = groupHandle;
		_musics[0].id = id;
		_musics[0].isResumable = false;
		_musics[1].handle = NXAUDIOENGINE_INVALID_HANDLE;
		// Play synchronously
		_engine.setPause(groupHandle, false);
	}
	else {
		_engine.destroyVoiceGroup(groupHandle);
	}
}

void NxAudioEngine::stopMusic(int channel, double time)
{
	NxAudioEngineMusic& music = _musics[channel];

	if (trace_all || trace_music) trace("NxAudioEngine::%s: channel %d, midi %d, time %f\n", __func__, channel, music.id, time);

	if (time > 0.0)
	{
		time /= gamehacks.getCurrentSpeedhack();
		_engine.fadeVolume(music.handle, 0.0f, time);
		_engine.scheduleStop(music.handle, time);
		_lastVolumeFadeEndTime = _engine.mStreamTime + time;
	}
	else
	{
		_engine.stop(music.handle);
	}

	if (_engine.isVoiceGroup(music.handle)) {
		_engine.destroyVoiceGroup(music.handle);
	}

	music.id = 0;
	music.handle = NXAUDIOENGINE_INVALID_HANDLE;
}

void NxAudioEngine::pauseMusic(int channel, double time, bool push)
{
	NxAudioEngineMusic& music = _musics[channel];

	if (trace_all || trace_music) trace("NxAudioEngine::%s: midi %d, time %f\n", __func__, music.id, time);

	if (time > 0.0)
	{
		time /= gamehacks.getCurrentSpeedhack();
		_engine.fadeVolume(music.handle, 0.0f, time);
		_engine.schedulePause(music.handle, time);
		_lastVolumeFadeEndTime = _engine.mStreamTime + time;
	}
	else
	{
		_engine.setPause(music.handle, true);
	}

	if (push && _engine.isValidVoiceHandle(music.handle)) {
		if (trace_all || trace_music) trace("NxAudioEngine::%s: push music onto the stack for later usage\n", __func__);

		// Save for later usage
		_musicStack.push(music);

		// Invalidate the current handle
		music.id = 0;
		music.handle = NXAUDIOENGINE_INVALID_HANDLE;
	}
}

void NxAudioEngine::resumeMusic(int channel, double time, bool pop, const PlayFlags &playFlags)
{
	NxAudioEngineMusic& music = _musics[channel];
	time /= gamehacks.getCurrentSpeedhack();

	if (pop) {
		// Restore the last known paused music
		NxAudioEngineMusic lastMusic = _musicStack.top();
		_musicStack.pop();

		if (!(playFlags & PlayFlagsDoNotPause) && music.isResumable) {
			pauseMusic(channel, time, true);
		}
		else {
			// Whatever is currently playing, just stop it
			// If the handle is still invalid, nothing will happen
			stopMusic(channel, time);
		}

		music = lastMusic;
	}

	if (trace_all || trace_music) trace("NxAudioEngine::%s: midi %d, time %f\n", __func__, music.id, time);

	// Play it again from where it was left off
	if (time > 0.0) {
		_engine.setVolume(music.handle, 0.0);
	}
	resetMusicVolume(channel, time);
	_engine.setPause(music.handle, false);
}

bool NxAudioEngine::isMusicPlaying(int channel)
{
	const NxAudioEngineMusic& music = _musics[channel];

	return (_engine.isValidVoiceHandle(music.handle)
		|| _engine.isVoiceGroup(music.handle))
		&& !_engine.getPause(music.handle);
}

uint32_t NxAudioEngine::currentMusicId(int channel)
{
	return _musics[channel].id;
}

void NxAudioEngine::setMusicMasterVolume(float volume, double time)
{
	_previousMusicMasterVolume = _musicMasterVolume;

	_musicMasterVolume = volume;

	resetMusicVolume(0, time);
	resetMusicVolume(1, time);
}

void NxAudioEngine::restoreMusicMasterVolume(double time)
{
	if (_previousMusicMasterVolume != _musicMasterVolume && _previousMusicMasterVolume >= 0.0f)
	{
		_musicMasterVolume = _previousMusicMasterVolume;

		// Set them equally so if this API is called again, nothing will happen
		_previousMusicMasterVolume = _musicMasterVolume;

		resetMusicVolume(0, time);
		resetMusicVolume(1, time);
	}
}

float NxAudioEngine::getMusicVolume(int channel)
{
	return _musics[channel].wantedMusicVolume;
}

bool NxAudioEngine::isMusicVolumeFadeFinished()
{
	return _engine.mStreamTime >= _lastVolumeFadeEndTime;
}

float NxAudioEngine::getMusicMasterVolume()
{
	return _musicMasterVolume < 0.0f ? 1.0f : _musicMasterVolume;
}

void NxAudioEngine::setMusicVolume(float volume, int channel, double time)
{
	_musics[channel].wantedMusicVolume = volume;

	resetMusicVolume(channel, time);
}

void NxAudioEngine::resetMusicVolume(int channel, double time)
{
	const NxAudioEngineMusic& music = _musics[channel];
	const float volume = music.wantedMusicVolume * getMusicMasterVolume();

	if (time > 0.0) {
		time /= gamehacks.getCurrentSpeedhack();
		_engine.fadeVolume(music.handle, volume, time);
		_lastVolumeFadeEndTime = _engine.mStreamTime + time;
	}
	else {
		_engine.setVolume(music.handle, volume);
	}
}

void NxAudioEngine::setMusicSpeed(float speed, int channel)
{
	_engine.setRelativePlaySpeed(_musics[channel].handle, speed);
}

void NxAudioEngine::setMusicLooping(bool looping, int channel)
{
	_engine.setLooping(_musics[channel].handle, looping);
}

// Voice
bool NxAudioEngine::canPlayVoice(const char* name)
{
	char filename[MAX_PATH];

	return getFilenameFullPath<const char*>(filename, name, NxAudioEngineLayer::NXAUDIOENGINE_VOICE);
}

bool NxAudioEngine::playVoice(const char* name, float volume)
{
	char filename[MAX_PATH];

	bool exists = getFilenameFullPath<const char *>(filename, name, NxAudioEngineLayer::NXAUDIOENGINE_VOICE);

	if (trace_all || trace_voice) trace("NxAudioEngine::%s: %s\n", __func__, filename);

	if (exists)
	{
		SoLoud::WavStream* voice = new SoLoud::WavStream();

		voice->load(filename);

		// Stop any previously playing voice
		if (_engine.isValidVoiceHandle(_voiceHandle)) _engine.stop(_voiceHandle);

		_voiceHandle = _engine.play(*voice, volume);

		return _engine.isValidVoiceHandle(_voiceHandle);
	}
	else
		return false;
}

void NxAudioEngine::stopVoice(double time)
{
	if (time > 0.0)
	{
		_engine.fadeVolume(_voiceHandle, 0, time);
		_engine.scheduleStop(_voiceHandle, time);
	}
	else
	{
		_engine.stop(_voiceHandle);
	}
}

bool NxAudioEngine::isVoicePlaying()
{
	return _engine.isValidVoiceHandle(_voiceHandle) && !_engine.getPause(_voiceHandle);
}