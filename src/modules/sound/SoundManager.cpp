/**
 * @file
 */

#include "SoundManager.h"
#include "SoundConfig.h"
#include "app/I18N.h"
#include "core/Log.h"
#include "core/Var.h"
#include <SDL3/SDL.h>

namespace sound {

/**
 * @brief Internal data for a loaded sound. The opaque SoundHandle points to one of these.
 */
struct SoundData {
	SDL_AudioSpec spec;
	uint8_t *buffer = nullptr;
	uint32_t length = 0;
	SDL_AudioStream *stream = nullptr;
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
	if (!_enabled->boolVal()) {
		return false;
	}
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		Log::warn("Failed to initialize SDL audio subsystem: %s", SDL_GetError());
		return false;
	}
	_initialized = true;

	// Recreate audio streams for any previously loaded sounds
	for (SoundData *data : _sounds) {
		if (data->stream == nullptr && data->buffer != nullptr) {
			data->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &data->spec, nullptr, nullptr);
			if (data->stream == nullptr) {
				Log::warn("Failed to recreate audio stream: %s", SDL_GetError());
			}
		}
	}

	return true;
}

void SoundManager::closeDevice() {
	// Destroy all audio streams before tearing down the subsystem
	for (SoundData *data : _sounds) {
		if (data->stream != nullptr) {
			SDL_DestroyAudioStream(data->stream);
			data->stream = nullptr;
		}
	}
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
	if (_enabled->isDirty()) {
		_enabled->markClean();
		if (_enabled->boolVal()) {
			ensureDevice();
		} else {
			closeDevice();
		}
	}
	if (_volume->isDirty()) {
		_volume->markClean();
	}
}

void SoundManager::shutdown() {
	// Free all tracked sounds before closing the device
	for (SoundData *data : _sounds) {
		if (data->stream != nullptr) {
			SDL_DestroyAudioStream(data->stream);
		}
		if (data->buffer != nullptr) {
			SDL_free(data->buffer);
		}
		delete data;
	}
	_sounds.clear();
	closeDevice();
}

SoundHandle SoundManager::loadSound(const core::String &path) {
	if (!_enabled->boolVal()) {
		Log::debug("Sound is disabled, sound %s is not loaded", path.c_str());
		return nullptr;
	}
	if (!ensureDevice()) {
		Log::warn("Sound device not available");
		return nullptr;
	}

	SoundData *data = new SoundData();

	if (!SDL_LoadWAV(path.c_str(), &data->spec, &data->buffer, &data->length)) {
		Log::warn("Failed to load WAV file '%s': %s", path.c_str(), SDL_GetError());
		delete data;
		return nullptr;
	}

	data->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &data->spec, nullptr, nullptr);
	if (data->stream == nullptr) {
		Log::warn("Failed to open audio device stream: %s", SDL_GetError());
		SDL_free(data->buffer);
		delete data;
		return nullptr;
	}

	_sounds.push_back(data);
	Log::debug("Loaded sound: %s (%u bytes)", path.c_str(), data->length);
	return (SoundHandle)data;
}

void SoundManager::freeSound(SoundHandle handle) {
	if (handle == nullptr) {
		return;
	}
	SoundData *data = (SoundData *)handle;

	// Remove from tracking list
	for (size_t i = 0; i < _sounds.size(); ++i) {
		if (_sounds[i] == data) {
			_sounds.erase(i);
			break;
		}
	}

	if (data->stream != nullptr) {
		SDL_DestroyAudioStream(data->stream);
	}
	if (data->buffer != nullptr) {
		SDL_free(data->buffer);
	}
	delete data;
}

void SoundManager::playSound(SoundHandle handle) {
	if (handle == nullptr) {
		return;
	}
	if (!_enabled->boolVal()) {
		return;
	}
	if (!_initialized) {
		return;
	}

	SoundData *data = (SoundData *)handle;

	if (data->stream == nullptr) {
		return;
	}
	SDL_ClearAudioStream(data->stream);
	if (!SDL_PutAudioStreamData(data->stream, data->buffer, (int)data->length)) {
		Log::warn("Failed to put audio stream data: %s", SDL_GetError());
		return;
	}
	const int volume = _volume->intVal();
	if (volume < 100) {
		SDL_SetAudioStreamGain(data->stream, volume / 100.0f);
	}
	SDL_ResumeAudioStreamDevice(data->stream);
}

} // namespace sound
