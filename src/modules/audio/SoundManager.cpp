/**
 * @file
 */

#include "SoundManager.h"
#include "core/command/Command.h"
#include "core/Var.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/GLM.h"
#include "core/GameConfig.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace audio {

SoundManager::Channel SoundManager::_channels[MAX_CHANNELS];

static inline double getAngleBetweenPoints(const glm::ivec2& pos1, const glm::ivec2& pos2) {
	const double dX = (double)(pos2.x - pos1.x);
	const double dY = (double)(pos2.y - pos1.y);
	const double angleInDegrees = glm::degrees(::atan2(dY, dX));
	return angleInDegrees;
}

SoundManager::SoundManager(const io::FilesystemPtr& filesystem) :
		_filesystem(filesystem) {
}

bool SoundManager::init() {
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
		Log::error(logid, "unable to initialize audio: %s", SDL_GetError());
		_state = SoundState::CLOSED;
		return false;
	}
	const int n = SDL_GetNumAudioDrivers();
	if (n == 0) {
		Log::error(logid, " no built-in audio drivers");
		_state = SoundState::CLOSED;
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	} else {
		for (int i = 0; i < n; ++i) {
			Log::info(logid, "available audio driver %s", SDL_GetAudioDriver(i));
		}
	}

	const int result = Mix_Init(MIX_INIT_OGG);
	if (!(result & MIX_INIT_OGG)) {
		Log::error(logid, "Failed to initialize sdl mixer with ogg support: %s", Mix_GetError());
	}

	Log::info(logid, "audio driver: %s", SDL_GetCurrentAudioDriver());

	const int audioRate = 44100;
	const Uint16 audioFormat = MIX_DEFAULT_FORMAT;
	const int audioChannels = MIX_DEFAULT_CHANNELS;
	const int audioBuffers = 4096;

	if (Mix_OpenAudio(audioRate, audioFormat, audioChannels, audioBuffers) != 0) {
		Log::error(logid, "unable to initialize mixer: %s", Mix_GetError());
		_state = SoundState::CLOSED;
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}

	Mix_AllocateChannels(MAX_CHANNELS);
	Mix_ChannelFinished(channelFinished);

	Log::info(logid, "sound initialized");

	_state = SoundState::INITIALIZED;
	volume(core::Var::get(cfg::AudioSoundVolume)->intVal());
	musicVolume(core::Var::get(cfg::AudioMusicVolume)->intVal());
	return true;
}

void SoundManager::construct() {
	_volume = core::Var::get(cfg::AudioSoundVolume, MIX_MAX_VOLUME / 4);
	_musicVolume = core::Var::get(cfg::AudioMusicVolume, MIX_MAX_VOLUME / 4);
}

void SoundManager::shutdown() {
	Mix_HaltMusic();
	Mix_FreeMusic(_music);
	_music = nullptr;
	Mix_AllocateChannels(0);
	for (auto i : _map) {
		Mix_FreeChunk(i->second);
	}
	_state = SoundState::CLOSED;
	_map.clear();
	Mix_CloseAudio();
	Mix_Quit();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SoundManager::channelFinished(int channel) {
	core_assert(channel >= 0);
	core_assert(channel < MAX_CHANNELS);
	core_memset(&_channels[channel], 0, sizeof(_channels[channel]));
}

void SoundManager::halt(int sound) {
	if (sound <= -1) {
		return;
	}

	Mix_HaltChannel(sound);
}

void SoundManager::haltAll() {
	Mix_HaltChannel(-1);
}

int SoundManager::playMusic(const core::String& music, bool loop) {
	if (!isActive()) {
		return -1;
	}
	if (music.empty()) {
		Log::error(logid, "no music file to play was provided");
		return -1;
	}
	if (_musicPlaying == music) {
		return -1;
	}

	Mix_HaltMusic();
	Mix_FreeMusic(_music);
	_music = nullptr;

	const core::String& fullPath = core::string::format("music/%s.ogg", music.c_str());
	const io::FilePtr& file = _filesystem->open(fullPath);
	if (!file->exists()) {
		Log::error(logid, "unable to open music file: %s", fullPath.c_str());
		return -1;
	}
	SDL_RWops* rwops = file->createRWops(io::FileMode::Read);
	if (rwops == nullptr) {
		Log::error(logid, "unable to load music file: %s", fullPath.c_str());
		return -1;
	}
	_music = Mix_LoadMUS_RW(rwops, 1);
	if (_music == nullptr) {
		Log::error(logid, "unable to load music file: %s", Mix_GetError());
		return -1;
	}

	const int ret = Mix_PlayMusic(_music, loop ? -1 : 1);
	if (ret == -1) {
		Log::error(logid, "unable to play music file: %s", Mix_GetError());
	} else {
		_musicPlaying = music;
	}
	return ret;
}

void SoundManager::haltMusic(int music) {
	if (!isActive()) {
		return;
	}

	if (music <= -1) {
		return;
	}

	Mix_HaltMusic();
	Mix_FreeMusic(_music);
	_music = nullptr;
	_musicPlaying = "";
}

int SoundManager::play(int channel, const core::String& filename, const glm::vec3& position, bool loop) {
	if (!isActive()) {
		return -1;
	}
	if (filename.empty()) {
		Log::error(logid, "no sound file to play was provided");
		return -1;
	}
	Mix_Chunk *sound = getChunk(filename);
	if (!sound) {
		return -1;
	}
	if (channel < -1 || channel >= MAX_CHANNELS) {
		return -1;
	}

	if (channel != -1 && Mix_Playing(channel)) {
		return -1;
	}
	channel = Mix_PlayChannel(channel, sound, loop ? -1 : 0);
	if (channel <= -1) {
		Log::error(logid, "unable to play sound file: %s", Mix_GetError());
	} else {
		core_assert(channel < MAX_CHANNELS);
		_channels[channel].channel = channel;
		_channels[channel].chunk = sound;
		_channels[channel].pos = position;
	}
	return channel;
}

Mix_Chunk* SoundManager::getChunk(const core::String& filename) {
	if (filename.empty()) {
		Log::error(logid, "no sound file to get the chunk for was provided");
		return nullptr;
	}
	auto i = _map.find(filename);
	if (i != _map.end()) {
		return i->second;
	}

	const core::String& fullPath = core::string::format("sound/%s.ogg", filename.c_str());
	io::FilePtr file = _filesystem->open(fullPath);
	if (!file->exists()) {
		Log::error(logid, "unable to open sound file: %s", fullPath.c_str());
		return nullptr;
	}
	SDL_RWops* rwops = file->createRWops(io::FileMode::Read);
	if (rwops == nullptr) {
		Log::error(logid, "unable to load sound file: %s", fullPath.c_str());
		return nullptr;
	}
	Mix_Chunk *sound = Mix_LoadWAV_RW(rwops, 1);
	_map.put(filename, sound);
	if (sound == nullptr) {
		Log::error(logid, "unable to load sound file %s: %s", filename.c_str(), Mix_GetError());
	}

	return sound;
}

void SoundManager::update(uint32_t deltaTime) {
	_time += deltaTime;

	if (_volume->isDirty()) {
		_volume->markClean();
		volume(_volume->intVal());
	}

	if (_musicVolume->isDirty()) {
		_musicVolume->markClean();
		musicVolume(_musicVolume->intVal());
	}

	if (!isActive()) {
		return;
	}
	const double scale = 5.0;
	for (int i = 0; i < MAX_CHANNELS; ++i) {
		Channel &channel = _channels[i];
		if (channel.chunk == nullptr) {
			continue;
		}
		const float dist = glm::distance(_listenerPosition, channel.pos);
		const double angleInDegrees = getAngleBetweenPoints(_listenerPosition, channel.pos);
		const int volume = core_max(0, MIX_MAX_VOLUME - (int)(dist * scale));
		Mix_SetPosition(channel.channel, static_cast<int16_t>(angleInDegrees), volume);
	}
}

void SoundManager::setListenerPosition(const glm::vec3& position, const glm::vec3& velocity) {
	_listenerPosition = position;
}

void SoundManager::pause() {
	if (!isActive()) {
		Log::info(logid, "sound is already paused");
		return;
	}
	Log::debug(logid, "sound is now paused");
	_state |= SoundState::PAUSE;
	Mix_PauseMusic();
	Mix_Pause(-1);
}

void SoundManager::resume() {
	if (isActive()) {
		Log::info(logid, "sound is already active");
		return;
	}
	Log::info(logid, "sound is active again");
	_state &= ~SoundState::PAUSE;
	Mix_ResumeMusic();
	Mix_Resume(-1);
}

bool SoundManager::isActive() const {
	if ((_state & SoundState::INITIALIZED) != SoundState::INITIALIZED) {
		return false;
	}
	if ((_state & SoundState::PAUSE) == SoundState::PAUSE) {
		return false;
	}
	return true;
}

int SoundManager::volume(int newVolume) {
	return Mix_Volume(-1, newVolume);
}

int SoundManager::musicVolume(int newVolume) {
	return Mix_VolumeMusic(newVolume);
}

}
