/**
 * @file
 */

#include "MementoHandler.h"

#include "core/ArrayLength.h"
#include "voxel/Voxel.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "command/Command.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include "core/Log.h"
#include "core/Zip.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxedit {

static const MementoState InvalidMementoState{MementoType::Max, MementoData(), -1, -1, "", voxel::Region::InvalidRegion, glm::mat4(1.0f), 0};

MementoData::MementoData(const uint8_t* buf, size_t bufSize,
		const voxel::Region& _region) :
		_compressedSize(bufSize), _region(_region) {
	if (buf != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = (uint8_t*)core_malloc(_compressedSize);
		core_memcpy(_buffer, buf, _compressedSize);
	} else {
		core_assert(_compressedSize == 0);
	}
}

MementoData::MementoData(MementoData&& o) noexcept :
		_compressedSize(o._compressedSize),
		_buffer(o._buffer),
		_region(o._region) {
	o._compressedSize = 0;
	o._buffer = nullptr;
}

MementoData::~MementoData() {
	if (_buffer != nullptr) {
		core_free(_buffer);
		_buffer = nullptr;
	}
}

MementoData::MementoData(const MementoData& o) :
		_compressedSize(o._compressedSize),
		_region(o._region) {
	if (o._buffer != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = (uint8_t*)core_malloc(_compressedSize);
		core_memcpy(_buffer, o._buffer, _compressedSize);
	} else {
		core_assert(_compressedSize == 0);
	}
}

MementoData& MementoData::operator=(MementoData &&o) noexcept {
	if (this != &o) {
		_compressedSize = o._compressedSize;
		o._compressedSize = 0;
		if (_buffer) {
			core_free(_buffer);
		}
		_buffer = o._buffer;
		o._buffer = nullptr;
		_region = o._region;
	}
	return *this;
}

MementoData MementoData::fromVolume(const voxel::RawVolume* volume) {
	if (volume == nullptr) {
		return MementoData();
	}
	const size_t uncompressedBufferSize = volume->region().voxels() * sizeof(voxel::Voxel);
	const uint32_t compressedBufferSize = core::zip::compressBound(uncompressedBufferSize);
	uint8_t* compressedBuf = (uint8_t*)core_malloc(compressedBufferSize);
	size_t finalBufSize = 0u;
	if (!core::zip::compress(volume->data(), uncompressedBufferSize, compressedBuf, compressedBufferSize, &finalBufSize)) {
		core_free(compressedBuf);
		return MementoData();
	}
	MementoData data(compressedBuf, finalBufSize, volume->region());
	core_free(compressedBuf);

	Log::debug("Memento state. Volume: %i, compressed: %i",
			(int)uncompressedBufferSize, (int)data._compressedSize);
	return data;
}

voxel::RawVolume* MementoData::toVolume(const MementoData& mementoData) {
	if (mementoData._buffer == nullptr) {
		return nullptr;
	}
	const size_t uncompressedBufferSize = mementoData._region.voxels() * sizeof(voxel::Voxel);
	uint8_t *uncompressedBuf = (uint8_t*)core_malloc(uncompressedBufferSize);
	if (!core::zip::uncompress(mementoData._buffer, mementoData._compressedSize, uncompressedBuf, uncompressedBufferSize)) {
		return nullptr;
	}
	return voxel::RawVolume::createRaw((voxel::Voxel*)uncompressedBuf, mementoData._region);
}

MementoHandler::MementoHandler() {
}

MementoHandler::~MementoHandler() {
}

bool MementoHandler::init() {
	return true;
}

void MementoHandler::shutdown() {
	clearStates();
}

void MementoHandler::lock() {
	++_locked;
}

void MementoHandler::unlock() {
	--_locked;
}

void MementoHandler::print() const {
	Log::info("Current memento state index: %i", _statePosition);
	int i = 0;

	const char *states[] = {
		"Modification",
		"SceneNodeMove",
		"SceneNodeAdded",
		"SceneNodeRemoved",
		"SceneNodeRenamed",
		"SceneNodeTransform"
	};
	static_assert((int)MementoType::Max == lengthof(states), "Array sizes don't match");

	for (const MementoState& state : _states) {
		const glm::ivec3& mins = state.region.getLowerCorner();
		const glm::ivec3& maxs = state.region.getUpperCorner();
		Log::info("%4i: (%s) node id: %i (parent: %i) (frame %i) - %s (%s) [mins(%i:%i:%i)/maxs(%i:%i:%i)] (size: %ib)",
				i++, states[(int)state.type], state.nodeId, state.parentId, state.keyFrame, state.name.c_str(), state.data._buffer == nullptr ? "empty" : "volume",
						mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z, (int)state.data.size());
	}
}

void MementoHandler::construct() {
	command::Command::registerCommand("ve_mementoinfo", [&] (const command::CmdArgs& args) {
		print();
	});
}

void MementoHandler::clearStates() {
	_states.clear();
	_statePosition = 0u;
}

MementoState MementoHandler::undo() {
	if (!canUndo()) {
		return InvalidMementoState;
	}
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	const MementoState& s = state();
	--_statePosition;
	if (s.type == MementoType::Modification) {
		for (int i = _statePosition; i >= 0; --i) {
			MementoState& prevS = _states[i];
			if ((prevS.type == MementoType::Modification || prevS.type == MementoType::SceneNodeAdded) && prevS.nodeId == s.nodeId) {
				core_assert(prevS.hasVolumeData());
				// use the region from the current state - but the volume from the previous state of this node
				return MementoState{s.type, prevS.data, s.parentId, s.nodeId, s.name, s.region, s.localMatrix, s.keyFrame};
			}
		}
		core_assert(_states[0].type == MementoType::Modification);
		return _states[0];
	}
	if (s.type == MementoType::SceneNodeTransform) {
		for (int i = _statePosition; i >= 0; --i) {
			MementoState& prevS = _states[i];
			if ((prevS.type == MementoType::SceneNodeTransform || prevS.type == MementoType::SceneNodeAdded || prevS.type == MementoType::Modification) &&
				prevS.nodeId == s.nodeId && prevS.keyFrame == s.keyFrame) {
				return MementoState{s.type, s.data, s.parentId, s.nodeId, s.name, s.region, prevS.localMatrix, s.keyFrame};
			}
		}
		return _states[0];
	}
	return s;
}

MementoState MementoHandler::redo() {
	if (!canRedo()) {
		return InvalidMementoState;
	}
	++_statePosition;
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	return state();
}

void MementoHandler::updateNodeId(int nodeId, int newNodeId) {
	for (MementoState& state : _states) {
		if (state.nodeId == nodeId) {
			state.nodeId = newNodeId;
		}
		if (state.parentId == nodeId) {
			state.parentId = newNodeId;
		}
	}
}

void MementoHandler::markNodeRemoved(const voxelformat::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxelformat::KeyFrameIndex keyFrameIdx = 0; // TODO: all key frames
	voxel::RawVolume *volume = node.volume();
	const voxelformat::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &transformMatrix = transform.worldMatrix();
	Log::debug("Mark node %i as deleted (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, name, volume, MementoType::SceneNodeRemoved, voxel::Region::InvalidRegion, transformMatrix, keyFrameIdx);
}

void MementoHandler::markNodeAdded(const voxelformat::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxelformat::KeyFrameIndex keyFrameIdx = 0; // TODO: all key frames
	voxel::RawVolume *volume = node.volume();
	const voxelformat::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &transformMatrix = transform.worldMatrix();
	Log::debug("Mark node %i as added (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, name, volume, MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, transformMatrix, keyFrameIdx);
}

void MementoHandler::markModification(const voxelformat::SceneGraphNode &node, const voxel::Region& modifiedRegion) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxelformat::KeyFrameIndex keyFrameIdx = 0;
	voxel::RawVolume *volume = node.volume();
	const voxelformat::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &transformMatrix = transform.worldMatrix();
	Log::debug("Mark node %i modification (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, name, volume, MementoType::Modification, modifiedRegion, transformMatrix, keyFrameIdx);
}

void MementoHandler::markNodeRenamed(const voxelformat::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	int frameId = 0;
	voxel::RawVolume *volume = node.volume();
	const voxelformat::SceneGraphTransform &transform = node.transform(frameId);
	const glm::mat4 &transformMatrix = transform.worldMatrix();
	Log::debug("Mark node %i renamed (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, name, volume, MementoType::SceneNodeRenamed, voxel::Region::InvalidRegion, transformMatrix, frameId);
}

void MementoHandler::markNodeMoved(int targetId, int sourceId) {
	markUndo(targetId, sourceId, "", nullptr, MementoType::SceneNodeMove, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
}

void MementoHandler::markNodeTransform(const voxelformat::SceneGraphNode &node, voxelformat::KeyFrameIndex keyFrameIdx) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	const voxelformat::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &localMatrix = transform.localMatrix();
	Log::debug("Mark node %i as translated (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, name, nullptr, MementoType::SceneNodeTransform, voxel::Region::InvalidRegion, localMatrix, keyFrameIdx);
}

bool MementoHandler::markUndoPreamble(int nodeId) {
	if (_locked > 0) {
		Log::debug("Don't add undo state - we are currently in locked mode");
		return false;
	}
	core_assert(nodeId >= 0);
	if (!_states.empty()) {
		// if we mark something as new undo state, we can throw away
		// every other state that follows the new one (everything after
		// the current state position)
		const size_t n = _states.size() - (_statePosition + 1);
		_states.erase_back(n);
	}
	return true;
}

void MementoHandler::markUndo(int parentId, int nodeId, const core::String &name, const voxel::RawVolume *volume,
							  MementoType type, const voxel::Region &region, const glm::mat4 &localMatrix,
							  voxelformat::KeyFrameIndex keyFrameIdx) {
	if (!markUndoPreamble(nodeId)) {
		return;
	}
	Log::debug("New undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(), (int)_states.size());
	voxel::logRegion("MarkUndo", region);
	const MementoData& data = MementoData::fromVolume(volume);
	_states.emplace_back(type, data, parentId, nodeId, name, region, localMatrix, keyFrameIdx);
	_statePosition = stateSize() - 1;
}
}
