/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Enum.h"
#include "core/io/Filesystem.h"
#include "core/collection/Map.h"
#include "core/SharedPtr.h"
#include <glm/vec3.hpp>

struct Mix_Chunk;
struct _Mix_Music;

#define MAX_CHANNELS 16

namespace audio {

enum class SoundState {
	CLOSED = 1 << 0, INITIALIZED = 1 << 1, PAUSE = 1 << 2
};
CORE_ENUM_BIT_OPERATIONS(SoundState)

/**
 * @note There should only be one sound manager instance
 */
class SoundManager: public core::IComponent {
private:
	static constexpr auto logid = Log::logid("SoundManager");
	io::FilesystemPtr _filesystem;
	core::StringMap<Mix_Chunk*> _map;
	core::VarPtr _volume;
	core::VarPtr _musicVolume;
	glm::vec3 _listenerPosition;
	_Mix_Music *_music = nullptr;
	core::String _musicPlaying;
	SoundState _state = SoundState::CLOSED;

	Mix_Chunk* getChunk(const core::String& filename);

	struct Channel {
		int channel;
		Mix_Chunk *chunk;
		glm::vec3 pos;
	};

	static Channel _channels[MAX_CHANNELS];
	static void channelFinished(int channel);

	inline bool isActive() const;

	int _play(int channel, const core::String& filename, const glm::vec3& position, bool loop, int millis);

public:
	SoundManager(const io::FilesystemPtr& filesystem);

	bool init();
	void construct();
	void shutdown();

	int playMusic(const core::String& music, bool loop = true);
	void haltMusic(int music);
	void halt(int channel);
	void haltAll();
	void pause();
	void resume();
	/**
	 * @param[in] channel If this is @c -1, the first free channel will be chosen
	 * @param[in] filename The name of the sound file - relative to @c sound/ and without extension
	 * @return Returns @c -1 on error, the channel otherwise.
	 */
	int play(int channel, const core::String& filename, const glm::vec3& position, bool loop);
	int playTimed(int channel, const core::String& filename, const glm::vec3& position, double seconds);
	void update();
	void setListenerPosition(const glm::vec3& position, const glm::vec3& velocity = glm::vec3(0.0f));
	int volume(int channel, int newVolume);
	int volume(int newVolume);
	int musicVolume(int newVolume);
};

using SoundManagerPtr = core::SharedPtr<SoundManager>;

}
