/**
 * @file
 */

#include "SoundManager.h"
#include "SoundConfig.h"
#include "app/I18N.h"
#include "core/Log.h"
#include "core/Var.h"
#include <SDL.h>

namespace sound {

/**
 * @brief Internal data for a loaded sound. The opaque SoundHandle points to one of these.
 */
struct SoundData {
	SDL_AudioSpec spec;
	uint8_t *buffer = nullptr;
	uint32_t length = 0;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_AudioStream *stream = nullptr;
#endif
};

SoundManager::~SoundManager() {
	shutdown();
}

void SoundManager::construct() {
	const core::VarDef soundEnabled(cfg::SoundEnabled, true, N_("Sound enabled"), N_("Enable or disable sound playback"));
	core::Var::registerVar(soundEnabled);
	const core::VarDef soundVolume(cfg::SoundVolume, 100, 0, 100, N_("Sound volume"), N_("Sound volume in percent (0-100)"));
	core::Var::registerVar(soundVolume);
}

bool SoundManager::ensureDevice() {
	if (_initialized) {
		return true;
	}
	if (_enabled && !_enabled->boolVal()) {
		return false;
	}
#if SDL_VERSION_ATLEAST(3, 2, 0)
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
#else
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
#endif
		Log::warn("Failed to initialize SDL audio subsystem: %s", SDL_GetError());
		return false;
	}
	_initialized = true;
	return true;
}

void SoundManager::closeDevice() {
#if !SDL_VERSION_ATLEAST(3, 2, 0)
	if (_device != 0) {
		SDL_CloseAudioDevice(_device);
		_device = 0;
	}
#endif
	if (_initialized) {
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		_initialized = false;
	}
}

bool SoundManager::init() {
	_enabled = core::getVar(cfg::SoundEnabled);
	_volume = core::getVar(cfg::SoundVolume);

	if (!_enabled->boolVal()) {
		Log::info("Sound is disabled");
		return true;
	}

	return ensureDevice();
}

void SoundManager::update(double /*nowSeconds*/) {
	if (_enabled && _enabled->isDirty()) {
		_enabled->markClean();
		if (_enabled->boolVal()) {
			ensureDevice();
		} else {
			closeDevice();
		}
	}
	if (_volume && _volume->isDirty()) {
		_volume->markClean();
	}
}

void SoundManager::shutdown() {
	closeDevice();
}

SoundHandle SoundManager::loadSound(const core::String &path) {
	if (!ensureDevice()) {
		Log::warn("Sound device not available - cannot load sound");
		return nullptr;
	}

	SoundData *data = new SoundData();

#if SDL_VERSION_ATLEAST(3, 2, 0)
	int length = 0;
	if (!SDL_LoadWAV(path.c_str(), &data->spec, &data->buffer, (Uint32 *)&length)) {
		Log::warn("Failed to load WAV file '%s': %s", path.c_str(), SDL_GetError());
		delete data;
		return nullptr;
	}
	data->length = (uint32_t)length;

	data->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &data->spec, nullptr, nullptr);
	if (data->stream == nullptr) {
		Log::warn("Failed to open audio device stream: %s", SDL_GetError());
		SDL_free(data->buffer);
		delete data;
		return nullptr;
	}
#else
	if (SDL_LoadWAV(path.c_str(), &data->spec, &data->buffer, &data->length) == nullptr) {
		Log::warn("Failed to load WAV file '%s': %s", path.c_str(), SDL_GetError());
		delete data;
		return nullptr;
	}

	if (_device == 0) {
		_device = SDL_OpenAudioDevice(nullptr, 0, &data->spec, &_deviceSpec, 0);
		if (_device == 0) {
			Log::warn("Failed to open audio device: %s", SDL_GetError());
			SDL_FreeWAV(data->buffer);
			delete data;
			return nullptr;
		}
	}
#endif

	Log::info("Loaded sound: %s (%u bytes)", path.c_str(), data->length);
	return (SoundHandle)data;
}

void SoundManager::freeSound(SoundHandle handle) {
	if (handle == nullptr) {
		return;
	}
	SoundData *data = (SoundData *)handle;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	if (data->stream != nullptr) {
		SDL_DestroyAudioStream(data->stream);
	}
	if (data->buffer != nullptr) {
		SDL_free(data->buffer);
	}
#else
	if (data->buffer != nullptr) {
		SDL_FreeWAV(data->buffer);
	}
#endif
	delete data;
}

void SoundManager::playSound(SoundHandle handle) {
	if (handle == nullptr) {
		return;
	}
	if (_enabled && !_enabled->boolVal()) {
		return;
	}
	if (!_initialized) {
		return;
	}

	SoundData *data = (SoundData *)handle;
	const int volume = _volume ? _volume->intVal() : 100;

#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_ClearAudioStream(data->stream);
	if (!SDL_PutAudioStreamData(data->stream, data->buffer, (int)data->length)) {
		Log::warn("Failed to put audio stream data: %s", SDL_GetError());
		return;
	}
	if (volume < 100) {
		SDL_SetAudioStreamGain(data->stream, volume / 100.0f);
	}
	SDL_ResumeAudioStreamDevice(data->stream);
#else
	SDL_ClearQueuedAudio(_device);
	if (SDL_QueueAudio(_device, data->buffer, data->length) < 0) {
		Log::warn("Failed to queue audio: %s", SDL_GetError());
		return;
	}
	SDL_PauseAudioDevice(_device, 0);
#endif
}

} // namespace sound
