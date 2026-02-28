/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/String.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include <SDL_audio.h>
#include <SDL_version.h>

namespace sound {

struct SoundData;

/**
 * @brief Opaque handle to a loaded sound
 */
using SoundHandle = void *;

/**
 * @brief Simple sound manager for playing WAV files.
 *
 * Supports loading multiple sounds via opaque handles. The audio device
 * is initialized lazily when it is first needed. All SDL audio resources
 * are properly torn down when the device is closed (e.g. when sound is
 * disabled at runtime) and recreated when re-enabled.
 */
class SoundManager : public core::IComponent {
private:
	core::DynamicArray<SoundData *> _sounds;
#if !SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_AudioDeviceID _device = 0;
	SDL_AudioSpec _deviceSpec;
#endif
	bool _initialized = false;
	core::VarPtr _volume;
	core::VarPtr _enabled;

	/**
	 * @brief Lazily initialize the audio device and recreate streams for tracked sounds
	 * @return true if the device is ready to use
	 */
	bool ensureDevice();
	/**
	 * @brief Tear down all SDL audio resources for tracked sounds and close the device.
	 * The PCM buffers are kept so sounds can be recreated on re-enable.
	 */
	void closeDevice();

public:
	SoundManager() = default;
	virtual ~SoundManager();

	void construct() override;
	bool init() override;
	void update(double nowSeconds);
	void shutdown() override;

	/**
	 * @brief Load a WAV file
	 * @param path The file path to the WAV file
	 * @return An opaque handle to the sound, or @c nullptr on failure
	 */
	SoundHandle loadSound(const core::String &path);

	/**
	 * @brief Free a previously loaded sound
	 * @param handle The sound handle obtained from @c loadSound
	 */
	void freeSound(SoundHandle handle);

	/**
	 * @brief Play a previously loaded sound
	 * @param handle The sound handle obtained from @c loadSound
	 */
	void playSound(SoundHandle handle);
};

} // namespace sound
