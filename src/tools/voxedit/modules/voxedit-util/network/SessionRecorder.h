/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "memento/IMementoStateListener.h"
#include "network/ProtocolMessage.h"

namespace voxedit {

class SceneManager;

/**
 * @brief Records a voxedit editing session to a .vrec file.
 *
 * Implements @c IMementoStateListener to capture every memento state change
 * and serializes the corresponding protocol messages to disk. The first
 * message written is always a full scene state so that playback can start
 * from a complete base.
 *
 * File format:
 *  - 4 bytes magic: "VREC"
 *  - 1 byte protocol version
 *  - N protocol messages (each: 4 bytes size + 1 byte type + payload)
 */
class SessionRecorder : public memento::IMementoStateListener {
private:
	SceneManager *_sceneMgr;
	io::FilePtr _file;
	io::FileStream *_stream = nullptr;
	bool _recording = false;
	core::String _filename;

	bool writeMessage(const network::ProtocolMessage &msg);

public:
	SessionRecorder(SceneManager *sceneMgr);
	~SessionRecorder();

	bool startRecording(const core::String &filename);
	void stopRecording();
	bool isRecording() const;
	const core::String &filename() const;

	void onMementoStateAdded(const memento::MementoState &state) override;
	void onMementoStateSkipped(const memento::MementoState &state) override;
};

inline const core::String &SessionRecorder::filename() const {
	return _filename;
}

} // namespace voxedit
