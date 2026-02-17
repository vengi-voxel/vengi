/**
 * @file
 */

#include "SessionRecorder.h"
#include "ProtocolVersion.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "memento/MementoHandler.h"
#include "protocol/NodeAddedMessage.h"
#include "protocol/NodeKeyFramesMessage.h"
#include "protocol/NodeMovedMessage.h"
#include "protocol/NodePaletteChangedMessage.h"
#include "protocol/NodeNormalPaletteChangedMessage.h"
#include "protocol/NodePropertiesMessage.h"
#include "protocol/NodeRemovedMessage.h"
#include "protocol/NodeRenamedMessage.h"
#include "protocol/NodeIKConstraintMessage.h"
#include "protocol/SceneStateMessage.h"
#include "protocol/VoxelModificationMessage.h"
#include "protocol/SceneGraphAnimationMessage.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

SessionRecorder::SessionRecorder(SceneManager *sceneMgr) : _sceneMgr(sceneMgr) {
}

SessionRecorder::~SessionRecorder() {
	stopRecording();
}

bool SessionRecorder::writeMessage(const network::ProtocolMessage &msg) {
	if (!_stream || !_recording) {
		return false;
	}
	const size_t total = msg.size();
	if (_stream->write(msg.getBuffer(), total) == -1) {
		Log::error("Failed to write protocol message of type %d (%u bytes) to recording file", msg.getId(),
				   (uint32_t)total);
		return false;
	}
	return true;
}

bool SessionRecorder::startRecording(const core::String &filename) {
	stopRecording();

	_filename = filename;
	_file = core::make_shared<io::File>(filename, io::FileMode::SysWrite);
	_stream = new io::FileStream(_file);
	if (!_stream->valid()) {
		Log::error("Failed to open recording file: %s", filename.c_str());
		delete _stream;
		_stream = nullptr;
		_file = {};
		return false;
	}

	// Write magic bytes
	if (!_stream->writeUInt32(FourCC('V', 'R', 'E', 'C'))) {
		Log::error("Failed to write magic bytes to recording file");
		delete _stream;
		_stream = nullptr;
		_file = {};
		return false;
	}

	// Write protocol version
	const uint8_t version = PROTOCOL_VERSION;
	if (_stream->writeUInt8(version) == false) {
		Log::error("Failed to write protocol version to recording file");
		delete _stream;
		_stream = nullptr;
		_file = {};
		return false;
	}

	_recording = true;

	// Write the full scene state as first message
	SceneStateMessage sceneState(_sceneMgr->sceneGraph());
	if (!writeMessage(sceneState)) {
		Log::error("Failed to write initial scene state to recording file");
		stopRecording();
		return false;
	}

	Log::info("Started recording to %s", filename.c_str());
	return true;
}

void SessionRecorder::stopRecording() {
	if (!_recording) {
		return;
	}
	_recording = false;
	if (_stream) {
		_stream->flush();
		delete _stream;
		_stream = nullptr;
	}
	_file = {};
	Log::info("Stopped recording");
}

bool SessionRecorder::isRecording() const {
	return _recording;
}

void SessionRecorder::onMementoStateAdded(const memento::MementoState &state) {
	if (!_recording) {
		return;
	}
	switch (state.type) {
	case memento::MementoType::Modification: {
		VoxelModificationMessage msg(state);
		writeMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeMove: {
		NodeMovedMessage msg(state);
		writeMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeAdded: {
		NodeAddedMessage msg(state);
		writeMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeRemoved: {
		NodeRemovedMessage msg(state);
		writeMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeRenamed: {
		NodeRenamedMessage msg(state);
		writeMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodePaletteChanged: {
		NodePaletteChangedMessage msg(state);
		writeMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeKeyFrames: {
		NodeKeyFramesMessage msg(state);
		writeMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeProperties: {
		NodePropertiesMessage msg(state);
		writeMessage(msg);
		break;
	}
	case memento::MementoType::SceneNodeIKConstraint: {
		NodeIKConstraintMessage ikMsg(state, _sceneMgr->sceneGraph());
		writeMessage(ikMsg);
		break;
	}
	case memento::MementoType::SceneNodeNormalPaletteChanged: {
		NodeNormalPaletteChangedMessage msg(state);
		writeMessage(msg);
		break;
	}
	case memento::MementoType::SceneGraphAnimation: {
		SceneGraphAnimationMessage msg(state);
		writeMessage(msg);
		break;
	}
	case memento::MementoType::Max:
		break;
	}
}

void SessionRecorder::onMementoStateSkipped(const memento::MementoState &state) {
	onMementoStateAdded(state);
}

} // namespace voxedit
