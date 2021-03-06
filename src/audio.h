/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 myst6re                                            //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2021 Julian Xhokaxhiu                                   //
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

#include <stack>
#include <string>
#include <vector>
#include <unordered_map>
#include <soloud/soloud.h>
#include <soloud/soloud_wav.h>
#include <soloud/soloud_wavstream.h>
#include "audio/vgmstream/vgmstream.h"
#include "audio/openpsf/openpsf.h"

#define NXAUDIOENGINE_INVALID_HANDLE 0xfffff000

class NxAudioEngine
{
public:
	enum MusicFlags {
		MusicFlagsNone = 0x0,
		MusicFlagsIsResumable = 0x1,
		MusicFlagsDoNotPause = 0x2
	};

	struct MusicOptions
	{
		MusicOptions() :
			offsetSeconds(0.0),
			flags(MusicFlagsNone),
			noIntro(false),
			fadetime(0.0),
			targetVolume(-1.0f),
			useNameAsFullPath(false),
			format("")
		{}
		SoLoud::time offsetSeconds;
		MusicFlags flags;
		bool noIntro;
		SoLoud::time fadetime;
		float targetVolume;
		bool useNameAsFullPath;
		char format[12];
	};

	struct NxAudioEngineSFX
	{
		NxAudioEngineSFX() :
			id(0),
			stream(nullptr),
			handle(NXAUDIOENGINE_INVALID_HANDLE),
			volume(1.0f),
			loop(false)
		{}
		int id;
		SoLoud::VGMStream *stream;
		SoLoud::handle handle;
		float volume;
		bool loop;
	};

	struct NxAudioEngineMusic
	{
		NxAudioEngineMusic() :
			handle(NXAUDIOENGINE_INVALID_HANDLE),
			id(0),
			isResumable(false),
			wantedMusicVolume(1.0f) {}
		SoLoud::handle handle;
		uint32_t id;
		bool isResumable;
		float wantedMusicVolume;
	};

	struct NxAudioEngineVoice
	{
		NxAudioEngineVoice() :
			handle(NXAUDIOENGINE_INVALID_HANDLE),
			stream(nullptr) {}
		SoLoud::handle handle;
		SoLoud::VGMStream* stream;
	};

	struct NxAudioEngineAmbient
	{
		NxAudioEngineAmbient() :
			handle(NXAUDIOENGINE_INVALID_HANDLE),
			stream(nullptr) {}
		SoLoud::handle handle;
		SoLoud::VGMStream* stream;
	};

private:
	enum NxAudioEngineLayer
	{
		NXAUDIOENGINE_SFX,
		NXAUDIOENGINE_MUSIC,
		NXAUDIOENGINE_VOICE,
		NXAUDIOENGINE_AMBIENT,
	};

	bool _engineInitialized = false;
	SoLoud::Soloud _engine;
	bool _openpsf_loaded = false;

	// SFX
	short _sfxReusableChannels = 0;
	short _sfxTotalChannels = 0;
	float _sfxMasterVolume = -1.0f;
	std::map<int, NxAudioEngineSFX> _sfxChannels;
	std::vector<int> _sfxSequentialIndexes;
	std::map<int, SoLoud::VGMStream*> _sfxEffectsHandler;

	SoLoud::VGMStream* loadSFX(int id, bool loop = false);
	void unloadSFXChannel(int channel);

	// MUSIC
	NxAudioEngineMusic _musics[2];
	std::stack<NxAudioEngineMusic> _musicStack; // For resuming

	float _previousMusicMasterVolume = -1.0f;
	float _musicMasterVolume = -1.0f;
	SoLoud::time _lastVolumeFadeEndTime = 0.0;

	SoLoud::AudioSource* loadMusic(const char* name, bool isFullPath = false, const char* format = nullptr);
	void overloadPlayArgumentsFromConfig(char* name, uint32_t *id, MusicOptions *MusicOptions);
	void resetMusicVolume(int channel, double time = 0);

	// VOICE
	NxAudioEngineVoice _currentVoice;

	// AMBIENT
	std::map<std::string, int> _ambientSequentialIndexes;
	NxAudioEngineAmbient _currentAmbient;

	// MISC
	// Returns false if the file does not exist
	template <class T>
	bool getFilenameFullPath(char *_out, T _key, NxAudioEngineLayer _type);

	bool fileExists(const char* filename);

	// CFG
	std::unordered_map<NxAudioEngineLayer,toml::parse_result> nxAudioEngineConfig;

	void loadConfig();

public:

	bool init();
	void flush();
	void cleanup();

	// SFX
	void unloadSFX(int id);
	bool canPlaySFX(int id);
	void playSFX(int id, int channel, float panning, bool loop = false);
	void stopSFX(int channel);
	void pauseSFX(int channel);
	void resumeSFX(int channel);
	bool isSFXPlaying(int channel);
	float getSFXMasterVolume();
	void setSFXMasterVolume(float volume, double time = 0);
	void setSFXVolume(int channel, float volume, double time = 0);
	void setSFXSpeed(int channel, float speed, double time = 0);
	void setSFXPanning(int channel, float panning, double time = 0);
	void setSFXReusableChannels(short num);
	void setSFXTotalChannels(short num);

	// Music
	bool canPlayMusic(const char* name);
	bool playMusic(const char* name, uint32_t id, int channel, MusicOptions& MusicOptions = MusicOptions());
	void playSynchronizedMusics(const std::vector<std::string>& names, uint32_t id, MusicOptions& MusicOptions = MusicOptions());
	void stopMusic(int channel, double time = 0);
	void pauseMusic(int channel, double time = 0, bool push = false);
	void resumeMusic(int channel, double time = 0, bool pop = false, const MusicFlags &MusicFlags = MusicFlags());
	bool isMusicPlaying(int channel);
	uint32_t currentMusicId(int channel);
	void setMusicMasterVolume(float volume, double time = 0);
	void restoreMusicMasterVolume(double time = 0);
	float getMusicVolume(int channel);
	bool isMusicVolumeFadeFinished();
	float getMusicMasterVolume();
	void setMusicVolume(float volume, int channel, double time = 0);
	void setMusicSpeed(float speed, int channel);
	void setMusicLooping(bool looping, int channel);

	// Voice
	bool canPlayVoice(const char* name);
	bool playVoice(const char* name, float volume = 1.0f);
	void stopVoice(double time = 0);
	bool isVoicePlaying();

	// Ambient
	bool canPlayAmbient(const char* name);
	bool playAmbient(const char* name, float volume = 1.0f);
	void stopAmbient(double time = 0);
	void pauseAmbient(double time = 0);
	void resumeAmbient(double time = 0);
	bool isAmbientPlaying();
};

NxAudioEngine::MusicFlags operator|(NxAudioEngine::MusicFlags flags, NxAudioEngine::MusicFlags other) {
	return static_cast<NxAudioEngine::MusicFlags>(static_cast<int>(flags) | static_cast<int>(other));
}

extern NxAudioEngine nxAudioEngine;
