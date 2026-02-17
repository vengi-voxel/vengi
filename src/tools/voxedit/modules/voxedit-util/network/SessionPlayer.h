/**
 * @file
 */

#pragma once

#include "ClientNetwork.h"
#include "ProtocolMessageFactory.h"
#include "core/String.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FileStream.h"

namespace voxedit {

class SceneManager;

/**
 * @brief Plays back a recorded .vrec voxedit editing session.
 *
 * Reads protocol messages from a .vrec file and dispatches them through
 * the client-side protocol handler registry one message per frame.
 *
 * The playback speed can be configured to control the delay between messages.
 * The speed value is the delay between messages in seconds.
 * A speed of 1.0 means one message per second, lower values mean faster playback.
 *
 * When in playback mode and a scene modification is detected (dirty flag),
 * the playback stops.
 */
class SessionPlayer {
private:
	SceneManager *_sceneMgr;
	ClientNetwork _network;
	io::FilePtr _file;
	io::FileStream *_fileStream = nullptr;
	network::MessageStream _messageStream;
	bool _playing = false;
	bool _paused = false;
	/** @brief Delay between messages in seconds */
	float _speed = 0.1f;
	float _delayAccum = 0.0f;

	/**
	 * @brief Process a single message from the file stream
	 * @return true if a message was processed, false if no more messages
	 */
	bool processNextMessage();

public:
	SessionPlayer(SceneManager *sceneMgr);
	~SessionPlayer();

	bool startPlayback(const core::String &filename);
	void stopPlayback();
	bool isPlaying() const;
	bool isPaused() const;
	void setPaused(bool paused);
	float speed() const;
	void setSpeed(float speed);

	/**
	 * @brief Called from SceneManager::update() each frame.
	 * Processes at most one message per frame, respecting playback speed.
	 */
	void update(double deltaSeconds);
};

} // namespace voxedit
