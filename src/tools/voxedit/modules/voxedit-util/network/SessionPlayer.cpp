/**
 * @file
 */

#include "SessionPlayer.h"
#include "ProtocolVersion.h"
#include "SessionRecorder.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "voxedit-util/SceneManager.h"
#include <string.h>

namespace voxedit {

SessionPlayer::SessionPlayer(SceneManager *sceneMgr) : _sceneMgr(sceneMgr), _network(sceneMgr) {
}

SessionPlayer::~SessionPlayer() {
	stopPlayback();
}

bool SessionPlayer::startPlayback(const core::String &filename) {
	stopPlayback();

	if (!_network.init()) {
		Log::error("Failed to initialize playback network handlers");
		return false;
	}

	_file = core::make_shared<io::File>(filename, io::FileMode::SysRead);
	_fileStream = new io::FileStream(_file);
	if (!_fileStream->valid()) {
		Log::error("Failed to open recording file for playback: %s", filename.c_str());
		delete _fileStream;
		_fileStream = nullptr;
		_file = {};
		_network.shutdown();
		return false;
	}

	// Read and validate magic bytes
	uint32_t magic;
	if (_fileStream->readUInt32(magic) == -1) {
		Log::error("Failed to read magic bytes from recording file");
		delete _fileStream;
		_fileStream = nullptr;
		_file = {};
		_network.shutdown();
		return false;
	}
	if (magic != FourCC('V', 'R', 'E', 'C')) {
		Log::error("Invalid recording file magic bytes");
		delete _fileStream;
		_fileStream = nullptr;
		_file = {};
		_network.shutdown();
		return false;
	}

	// Read and validate protocol version
	uint8_t version;
	if (_fileStream->readUInt8(version) == -1) {
		Log::error("Failed to read protocol version from recording file");
		delete _fileStream;
		_fileStream = nullptr;
		_file = {};
		_network.shutdown();
		return false;
	}
	if (version != PROTOCOL_VERSION) {
		Log::error("Protocol version mismatch: file has %d, expected %d", version, PROTOCOL_VERSION);
		delete _fileStream;
		_fileStream = nullptr;
		_file = {};
		_network.shutdown();
		return false;
	}

	_playing = true;
	_paused = false;
	_delayAccum = 0.0f;

	Log::info("Started playback from %s", filename.c_str());

	// Process the first message immediately (should be full scene state)
	processNextMessage();

	return true;
}

void SessionPlayer::stopPlayback() {
	if (!_playing) {
		return;
	}
	_playing = false;
	_paused = false;
	if (_fileStream) {
		delete _fileStream;
		_fileStream = nullptr;
	}
	_file = {};
	_messageStream.seek(0);
	_messageStream.reset();
	_network.shutdown();
	Log::info("Stopped playback");
}

bool SessionPlayer::isPlaying() const {
	return _playing;
}

bool SessionPlayer::isPaused() const {
	return _paused;
}

void SessionPlayer::setPaused(bool paused) {
	_paused = paused;
}

float SessionPlayer::speed() const {
	return _speed;
}

void SessionPlayer::setSpeed(float speed) {
	_speed = speed;
}

bool SessionPlayer::processNextMessage() {
	if (!_fileStream || _fileStream->eos()) {
		Log::info("Playback reached end of file");
		stopPlayback();
		return false;
	}

	// Read the next message header (4 bytes size + 1 byte type)
	// and body from the file stream into the message stream
	int32_t msgSize;
	if (_fileStream->readInt32(msgSize) == -1) {
		if (_fileStream->eos()) {
			Log::info("Playback reached end of file");
			stopPlayback();
			return false;
		}
		Log::error("Failed to read message size during playback");
		stopPlayback();
		return false;
	}

	uint8_t msgType;
	if (_fileStream->readUInt8(msgType) == -1) {
		Log::error("Failed to read message type during playback");
		stopPlayback();
		return false;
	}

	// Write the header and body into the message stream for deserialization
	_messageStream.reset();
	_messageStream.writeInt32(msgSize);
	_messageStream.writeUInt8(msgType);

	if (msgSize > 0) {
		// Read the message body from file
		uint8_t *buf = new uint8_t[msgSize];
		if (_fileStream->read(buf, msgSize) == -1) {
			Log::error("Failed to read message body during playback (%d bytes)", msgSize);
			delete[] buf;
			stopPlayback();
			return false;
		}
		_messageStream.write(buf, msgSize);
		delete[] buf;
	}

	// Now deserialize and dispatch the message
	if (ProtocolMessageFactory::isNewMessageAvailable(_messageStream)) {
		core::ScopedPtr<network::ProtocolMessage> msg(ProtocolMessageFactory::create(_messageStream));
		if (!msg) {
			Log::warn("Failed to deserialize message during playback");
			return false;
		}

		// Lock the recorder listener to prevent recording during playback
		Client &client = _sceneMgr->client();
		client.lockListener();
		if (network::ProtocolHandler *handler = _network.protocolHandler(*msg)) {
			handler->execute(0, *msg);
		} else {
			Log::debug("No handler for playback message type %d", (int)msg->getId());
		}
		client.unlockListener();
		return true;
	}

	Log::warn("Message not available after writing to stream");
	return false;
}

void SessionPlayer::update(double deltaSeconds) {
	if (!_playing || _paused) {
		return;
	}

	// If the scene became dirty, pause the playback
	if (_sceneMgr->dirty()) {
		Log::info("Scene modified during playback - pausing");
		_paused = true;
		return;
	}

	_delayAccum += (float)deltaSeconds;
	// _speed is the delay between messages in seconds
	if (_delayAccum < _speed) {
		return;
	}
	_delayAccum -= _speed;

	// Process at most one message per frame
	// Clear dirty flag after processing so we can detect external modifications
	// on the next update. Playback itself sets dirty via protocol handlers -
	// that's expected and not an external modification.
	processNextMessage();
	_sceneMgr->clearDirty();
}

} // namespace voxedit
