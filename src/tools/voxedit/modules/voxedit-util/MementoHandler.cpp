/**
 * @file
 */

#include "MementoHandler.h"

#include "core/ArrayLength.h"
#include "core/Optional.h"
#include "core/ScopedPtr.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "voxel/Palette.h"
#include "voxel/Voxel.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "command/Command.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include "core/Log.h"
#include "core/Zip.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

static const MementoState InvalidMementoState{MementoType::Max, MementoData(), -1, -1, "", voxel::Region::InvalidRegion, glm::mat4(1.0f), 0};

MementoData::MementoData(uint8_t *buf, size_t bufSize, const voxel::Region &_region)
	: _compressedSize(bufSize), _region(_region) {
	if (buf != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = buf;
	} else {
		core_assert(_compressedSize == 0);
	}
}

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

MementoData MementoData::fromVolume(const voxel::RawVolume* volume, const voxel::Region &region) {
	if (volume == nullptr) {
		return MementoData();
	}
	voxel::Region mementoRegion = region;
	const bool partialMemento = false; // mementoRegion.isValid(); TODO: see issue #200
	if (!partialMemento) {
		mementoRegion = volume->region();
	}

	const int allVoxels = volume->region().voxels();
	const uint32_t compressedBufferSize = core::zip::compressBound(allVoxels * sizeof(voxel::Voxel));
	io::BufferedReadWriteStream outStream(compressedBufferSize);
	io::ZipWriteStream stream(outStream);
	if (partialMemento) {
		voxel::RawVolume v(volume, region);
		stream.write(v.data(), allVoxels * sizeof(voxel::Voxel));
	} else {
		stream.write(volume->data(), allVoxels * sizeof(voxel::Voxel));
	}
	stream.flush();
	const size_t size = (size_t)outStream.size();
	return {outStream.release(), size, mementoRegion};
}

bool MementoData::toVolume(voxel::RawVolume* volume, const MementoData& mementoData) {
	if (mementoData._buffer == nullptr) {
		return false;
	}
	core_assert_always(volume != nullptr);
	if (volume == nullptr) {
		return false;
	}
	const size_t uncompressedBufferSize = mementoData.region().voxels() * sizeof(voxel::Voxel);
	io::MemoryReadStream dataStream(mementoData._buffer, mementoData._compressedSize);
	io::ZipReadStream stream(dataStream, (int)dataStream.size());
	uint8_t *uncompressedBuf = (uint8_t*)core_malloc(uncompressedBufferSize);
	if (stream.read(uncompressedBuf, uncompressedBufferSize) == -1) {
		core_free(uncompressedBuf);
		return false;
	}
	core::ScopedPtr<voxel::RawVolume> v(voxel::RawVolume::createRaw((voxel::Voxel*)uncompressedBuf, mementoData.region()));
	voxelutil::copyIntoRegion(*v, *volume, mementoData.region());
	return true;
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

const char *MementoHandler::typeToString(MementoType type) {
	const char *states[] = {
		"Modification",
		"SceneNodeMove",
		"SceneNodeAdded",
		"SceneNodeRemoved",
		"SceneNodeRenamed",
		"SceneNodeTransform",
		"SceneNodePaletteChanged",
		"SceneNodeKeyFrames",
		"SceneNodeProperties",
		"PaletteChanged"
	};
	static_assert((int)MementoType::Max == lengthof(states), "Array sizes don't match");
	return states[(int)type];
}

void MementoHandler::printState(const MementoState &state) const {
	const glm::ivec3& mins = state.region.getLowerCorner();
	const glm::ivec3& maxs = state.region.getUpperCorner();
	core::String palHash;
	if (state.palette.hasValue()) {
		palHash = core::string::toString(state.palette.value()->hash());
	}
	Log::info("%s: node id: %i (parent: %i) (frame %i) - %s (%s) [mins(%i:%i:%i)/maxs(%i:%i:%i)] (size: %ib) (palette: %s [hash: %s])",
			typeToString(state.type), state.nodeId, state.parentId, state.keyFrameIdx, state.name.c_str(), state.data._buffer == nullptr ? "empty" : "volume",
					mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z, (int)state.data.size(), state.palette.hasValue() ? "true" : "false", palHash.c_str());

}

void MementoHandler::print() const {
	Log::info("Current memento state index: %i", _statePosition);

	for (const MementoState& state : _states) {
		printState(state);
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

MementoState MementoHandler::undoModification(const MementoState &s) {
	core_assert(s.hasVolumeData());
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if ((prevS.type == MementoType::Modification || prevS.type == MementoType::SceneNodeAdded) &&
			prevS.nodeId == s.nodeId) {
			core_assert(prevS.hasVolumeData());
			voxel::logRegion("Undo current", s.region);
			voxel::logRegion("Undo previous", prevS.region);
			voxel::logRegion("Undo current data", s.data.region());
			voxel::logRegion("Undo previous data", prevS.data.region());
			// use the region from the current state - but the volume from the previous state of this node
			return MementoState{s.type, prevS.data, s.parentId, s.nodeId, s.name, s.region, s.worldMatrix, s.keyFrameIdx};
		}
	}
	core_assert(_states[0].type == MementoType::Modification);
	return _states[0];
}

MementoState MementoHandler::undoTransform(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if ((prevS.type == MementoType::SceneNodeTransform || prevS.type == MementoType::SceneNodeAdded ||
			 prevS.type == MementoType::Modification) &&
			prevS.nodeId == s.nodeId && prevS.keyFrameIdx == s.keyFrameIdx) {
			return MementoState{s.type,		s.data,	  s.parentId, s.nodeId, s.name, s.region, prevS.worldMatrix,
								s.keyFrameIdx, s.palette};
		}
	}
	return _states[0];
}

MementoState MementoHandler::undoPaletteChange(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (prevS.palette.hasValue()) {
			return MementoState{s.type,	  s.data,		 s.parentId,	s.nodeId,	  s.name,
								s.region, s.worldMatrix, s.keyFrameIdx, prevS.palette};
		}
	}
	return _states[0];
}

MementoState MementoHandler::undoNodeProperties(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (prevS.properties.hasValue()) {
			return MementoState{s.type, s.data, s.parentId, s.nodeId, s.name, s.region, s.keyFrames, s.palette, prevS.properties};
		}
	}
	return _states[0];
}

MementoState MementoHandler::undoKeyFrames(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (prevS.keyFrames.hasValue()) {
			return MementoState{s.type, s.data, s.parentId, s.nodeId, s.name, s.region, prevS.keyFrames, s.palette};
		}
	}
	return _states[0];
}

MementoState MementoHandler::undoRename(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (prevS.palette.hasValue()) {
			return MementoState{s.type,	  s.data,		 s.parentId,	s.nodeId, prevS.name,
								s.region, s.worldMatrix, s.keyFrameIdx, s.palette};
		}
	}
	return _states[0];
}

MementoState MementoHandler::undo() {
	if (!canUndo()) {
		return InvalidMementoState;
	}
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	const MementoState& s = state();
	--_statePosition;
	if (s.type == MementoType::Modification) {
		return undoModification(s);
	} else if (s.type == MementoType::SceneNodeTransform) {
		return undoTransform(s);
	} else if (s.type == MementoType::SceneNodePaletteChanged) {
		return undoPaletteChange(s);
	} else if (s.type == MementoType::SceneNodeProperties) {
		return undoNodeProperties(s);
	} else if (s.type == MementoType::SceneNodeKeyFrames) {
		return undoKeyFrames(s);
	} else if (s.type == MementoType::SceneNodeRenamed) {
		return undoRename(s);
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

void MementoHandler::markNodePropertyChange(const voxelformat::SceneGraphNode &node) {
	const int nodeId = node.id();
	if (!markUndoPreamble(nodeId)) {
		return;
	}
	const int parentId = node.parent();
	const core::String &name = node.name();
	Log::debug("New node property undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(), (int)_states.size());
	core::Optional<voxelformat::SceneGraphNodeProperties> properties;
	properties.setValue(node.properties());
	MementoState state(MementoType::SceneNodeProperties, parentId, nodeId, name, properties);
	addState(core::move(state));
}

void MementoHandler::markKeyFramesChange(const voxelformat::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxel::RawVolume *volume = nullptr;
	Log::debug("Mark node %i key frame changes (%s)", nodeId, name.c_str());
	markUndoKeyFrames(parentId, nodeId, name, volume, MementoType::SceneNodeKeyFrames, voxel::Region::InvalidRegion, node.keyFrames());
}

void MementoHandler::markNodeRemoved(const voxelformat::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxel::RawVolume *volume = node.volume();
	Log::debug("Mark node %i as deleted (%s)", nodeId, name.c_str());
	core::Optional<voxel::Palette> palette;
	palette.setValue(node.palette());
	Log::debug("palette node added hash: %lu", node.palette().hash());
	markUndoKeyFrames(parentId, nodeId, name, volume, MementoType::SceneNodeRemoved, voxel::Region::InvalidRegion, node.keyFrames(), palette);
}

void MementoHandler::markNodeAdded(const voxelformat::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxel::RawVolume *volume = node.volume();
	Log::debug("Mark node %i as added (%s)", nodeId, name.c_str());
	core::Optional<voxel::Palette> palette;
	palette.setValue(node.palette());
	Log::debug("palette node added hash: %lu", node.palette().hash());
	markUndoKeyFrames(parentId, nodeId, name, volume, MementoType::SceneNodeAdded, voxel::Region::InvalidRegion, node.keyFrames(), palette);
}

void MementoHandler::markInitialNodeState(const voxelformat::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxel::RawVolume *volume = node.volume();
	Log::debug("Mark node %i modification (%s)", nodeId, name.c_str());
	core::Optional<voxel::Palette> palette;
	if (_states.empty()) {
		palette.setValue(node.palette());
		Log::debug("palette modification hash: %lu", node.palette().hash());
	}
	core::Optional<voxelformat::SceneGraphNodeProperties> properties;
	properties.setValue(node.properties());
	markUndoKeyFrames(parentId, nodeId, name, volume, MementoType::Modification, voxel::Region::InvalidRegion, node.keyFrames(), palette, properties);
}

void MementoHandler::markModification(const voxelformat::SceneGraphNode &node, const voxel::Region& modifiedRegion) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxel::RawVolume *volume = node.volume();
	Log::debug("Mark node %i modification (%s)", nodeId, name.c_str());
	core::Optional<voxel::Palette> palette;
	if (_states.empty()) {
		palette.setValue(node.palette());
		Log::debug("palette modification hash: %lu", node.palette().hash());
	}
	markUndo(parentId, nodeId, name, volume, MementoType::Modification, modifiedRegion, glm::mat4(1.0f), 0, palette);
}

void MementoHandler::markPaletteChange(const voxelformat::SceneGraphNode &node, const voxel::Region& modifiedRegion) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxelformat::KeyFrameIndex keyFrameIdx = 0;
	voxel::RawVolume *volume = nullptr;
	if (modifiedRegion.isValid()) {
		volume = node.volume();
	}
	const voxelformat::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &transformMatrix = transform.worldMatrix();
	Log::debug("Mark node %i palette change (%s)", nodeId, name.c_str());
	core::Optional<voxel::Palette> palette;
	palette.setValue(node.palette());
	Log::debug("palette change hash: %lu", node.palette().hash());
	markUndo(parentId, nodeId, name, volume, MementoType::SceneNodePaletteChanged, modifiedRegion, transformMatrix, keyFrameIdx, palette);
}

void MementoHandler::markNodeRenamed(const voxelformat::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxelformat::KeyFrameIndex keyFrameIdx = 0;
	voxel::RawVolume *volume = node.volume();
	const voxelformat::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &transformMatrix = transform.worldMatrix();
	Log::debug("Mark node %i renamed (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, name, volume, MementoType::SceneNodeRenamed, voxel::Region::InvalidRegion, transformMatrix, keyFrameIdx);
}

void MementoHandler::markNodeMoved(int targetId, int sourceId) {
	markUndo(targetId, sourceId, "", nullptr, MementoType::SceneNodeMove, voxel::Region::InvalidRegion, glm::mat4(1.0f), -1);
}

void MementoHandler::markNodeTransform(const voxelformat::SceneGraphNode &node, voxelformat::KeyFrameIndex keyFrameIdx) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	const voxelformat::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &worldMatrix = transform.worldMatrix();
	Log::debug("Mark node %i as translated (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, name, nullptr, MementoType::SceneNodeTransform, voxel::Region::InvalidRegion, worldMatrix, keyFrameIdx);
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
							  MementoType type, const voxel::Region &region, const glm::mat4 &worldMatrix,
							  voxelformat::KeyFrameIndex keyFrameIdx, const core::Optional<voxel::Palette> &palette) {
	if (!markUndoPreamble(nodeId)) {
		return;
	}
	Log::debug("New undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(), (int)_states.size());
	voxel::logRegion("MarkUndo", region);
	const MementoData& data = MementoData::fromVolume(volume, region);
	MementoState state(type, data, parentId, nodeId, name, region, worldMatrix, keyFrameIdx, palette);
	addState(core::move(state));
}

void MementoHandler::markUndoKeyFrames(int parentId, int nodeId, const core::String &name,
									   const voxel::RawVolume *volume, MementoType type, const voxel::Region &region,
									   const voxelformat::SceneGraphKeyFrames &keyFrames,
									   const core::Optional<voxel::Palette> &palette,
									   const core::Optional<voxelformat::SceneGraphNodeProperties> &properties) {
	if (!markUndoPreamble(nodeId)) {
		return;
	}
	Log::debug("New undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(), (int)_states.size());
	voxel::logRegion("MarkUndo", region);
	const MementoData& data = MementoData::fromVolume(volume, region);
	core::Optional<voxelformat::SceneGraphKeyFrames> kf;
	kf.setValue(keyFrames);
	MementoState state(type, data, parentId, nodeId, name, region, kf, palette, properties);
	addState(core::move(state));
}

void MementoHandler::addState(MementoState &&state) {
	_states.emplace_back(state);
	_statePosition = stateSize() - 1;
}

}
