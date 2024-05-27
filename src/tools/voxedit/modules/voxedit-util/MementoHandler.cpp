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
#include "palette/Palette.h"
#include "voxel/Voxel.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "command/Command.h"
#include "core/Assert.h"
#include "core/StandardLib.h"
#include "core/Log.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelutil/VoxelUtil.h"
#include <inttypes.h>

namespace voxedit {

static const MementoState InvalidMementoState{MementoType::Max,
											  MementoData(),
											  InvalidNodeId,
											  InvalidNodeId,
											  InvalidNodeId,
											  {},
											  scenegraph::SceneGraphNodeType::Max,
											  voxel::Region::InvalidRegion,
											  {},
											  {},
											  0,
											  {}};

MementoData::MementoData(uint8_t *buf, size_t bufSize, const voxel::Region &region)
	: _compressedSize(bufSize), _region(region) {
	if (buf != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = buf;
	} else {
		core_assert(_compressedSize == 0);
	}
}

MementoData::MementoData(const uint8_t* buf, size_t bufSize,
		const voxel::Region& region) :
		_compressedSize(bufSize), _region(region) {
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

MementoData &MementoData::operator=(const MementoData &o) noexcept {
	if (this != &o) {
		_compressedSize = o._compressedSize;
		if (_buffer) {
			core_free(_buffer);
			_buffer = nullptr;
		}
		if (o._buffer != nullptr) {
			core_assert(_compressedSize > 0);
			_buffer = (uint8_t *)core_malloc(_compressedSize);
			core_memcpy(_buffer, o._buffer, _compressedSize);
		} else {
			core_assert(_compressedSize == 0);
		}
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
	io::BufferedReadWriteStream outStream(allVoxels * sizeof(voxel::Voxel));
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

void MementoHandler::beginGroup() {
	Log::debug("Begin memento group");
	_groupState.setValue(MementoState{});
}

void MementoHandler::endGroup() {
	Log::debug("End memento group");
	core_assert(_groupState.hasValue());
	addState(core::move(*_groupState.value()));
	_groupState.setValue(nullptr);
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
	Log::info("%s: node id: %i", typeToString(state.type), state.nodeId);
	Log::info(" - parent: %i", state.parentId);
	Log::info(" - key frame index: %i", state.keyFrameIdx);
	Log::info(" - name: %s", state.name.c_str());
	Log::info(" - worldMatrix");
	if (state.worldMatrix.hasValue()) {
		const glm::mat4 &m = *state.worldMatrix.value();
		Log::info("   - %f:%f:%f:%f", m[0][0], m[0][1], m[0][2], m[0][3]);
		Log::info("   - %f:%f:%f:%f", m[1][0], m[1][1], m[1][2], m[1][3]);
		Log::info("   - %f:%f:%f:%f", m[2][0], m[2][1], m[2][2], m[2][3]);
		Log::info("   - %f:%f:%f:%f", m[3][0], m[3][1], m[3][2], m[3][3]);
		Log::info(" - volume: %s", state.data._buffer == nullptr ? "empty" : "volume");
		Log::info(" - region: mins(%i:%i:%i)/maxs(%i:%i:%i)", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		Log::info(" - size: %ib", (int)state.data.size());
		Log::info(" - palette: %s [hash: %s]", state.palette.hasValue() ? "true" : "false", palHash.c_str());
	} else {
		Log::info(" - none");
	}
	if (state.pivot.hasValue()) {
		Log::info(" - pivot: %f:%f:%f", state.pivot->x, state.pivot->y, state.pivot->z);
	} else {
		Log::info(" - pivot: none");
	}
	if (state.keyFrames.hasValue()) {
		const scenegraph::SceneGraphKeyFramesMap &keyFrames = *state.keyFrames.value();
		Log::info(" - key frames");
		for (const auto &e : keyFrames) {
			Log::info("   - animation: %s", e->first.c_str());
			const scenegraph::SceneGraphKeyFrames &frames = e->second;
			for (const auto &f : frames) {
				Log::info("     - frame: %i", f.frameIdx);
				Log::info("       - interpolation: %s", scenegraph::InterpolationTypeStr[(int)f.interpolation]);
				Log::info("       - long rotation: %s", f.longRotation ? "true" : "false");
				Log::info("       - transform");
				const glm::mat4 &m = f.transform().worldMatrix();
				Log::info("         - %f:%f:%f:%f", m[0][0], m[0][1], m[0][2], m[0][3]);
				Log::info("         - %f:%f:%f:%f", m[1][0], m[1][1], m[1][2], m[1][3]);
				Log::info("         - %f:%f:%f:%f", m[2][0], m[2][1], m[2][2], m[2][3]);
				Log::info("         - %f:%f:%f:%f", m[3][0], m[3][1], m[3][2], m[3][3]);
			}
		}
	} else {
		Log::info(" - key frames: none");
	}
	if (state.properties.hasValue()) {
		const scenegraph::SceneGraphNodeProperties &props = *state.properties.value();
		Log::info(" - properties");
		for (const auto &e : props) {
			Log::info("   - %s: %s", e->first.c_str(), e->second.c_str());
		}
	} else {
		Log::info(" - properties: none");
	}
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
	// TODO: memento group - finish implementation see https://github.com/vengi-voxel/vengi/issues/376
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (prevS.nodeId != s.nodeId) {
			continue;
		}
		if (prevS.type == MementoType::Modification || prevS.type == MementoType::SceneNodeAdded) {
			core_assert(prevS.hasVolumeData() || prevS.referenceId != InvalidNodeId);
			voxel::logRegion("Undo current", s.region);
			voxel::logRegion("Undo previous", prevS.region);
			voxel::logRegion("Undo current data", s.data.region());
			voxel::logRegion("Undo previous data", prevS.data.region());
			// use the region from the current state - but the volume and palette from the previous state of this node
			return MementoState{s.type,		prevS.data, s.parentId, s.nodeId,	   prevS.referenceId, s.name,
								prevS.nodeType, s.region,	s.pivot,	s.worldMatrix, s.keyFrameIdx, s.palette};
		}
	}

	core_assert_msg(
		_states[0].type == MementoType::Modification ||
			(_states[0].referenceId != InvalidNodeId && _states[0].type == MementoType::SceneNodeAdded),
		"Expected to have a modification or scene node added with a reference state at the beginning, but got %i",
		(int)_states[0].type);
	return _states[0];
}

MementoState MementoHandler::undoTransform(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (prevS.nodeId != s.nodeId) {
			continue;
		}
		if ((prevS.type == MementoType::SceneNodeTransform || prevS.type == MementoType::Modification) &&
			prevS.keyFrameIdx == s.keyFrameIdx) {
			return MementoState{s.type,		s.data,	  s.parentId, s.nodeId,			 s.referenceId, s.name,
								s.nodeType, s.region, s.pivot,	  prevS.worldMatrix, s.keyFrameIdx, s.palette};
		}
		if (prevS.type == MementoType::SceneNodeAdded && prevS.keyFrames.hasValue()) {
			for (const auto &e : *prevS.keyFrames.value()) {
				for (const auto &f : e->second) {
					if (f.frameIdx == s.keyFrameIdx) {
						return MementoState{
							s.type,		   s.data,	   s.parentId, s.nodeId, s.referenceId,
							s.name,		   s.nodeType, s.region,   s.pivot,	 f.transform().worldMatrix(),
							s.keyFrameIdx, s.palette};
					}
				}
			}
		}
	}
	return _states[0];
}

MementoState MementoHandler::undoPaletteChange(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (prevS.palette.hasValue() && prevS.nodeId == s.nodeId) {
			return MementoState{s.type,		s.data,	  s.parentId, s.nodeId,		 s.referenceId, s.name,
								s.nodeType, s.region, s.pivot,	  s.worldMatrix, s.keyFrameIdx, prevS.palette};
		}
	}
	return _states[0];
}

MementoState MementoHandler::undoNodeProperties(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (prevS.properties.hasValue() && prevS.nodeId == s.nodeId) {
			return MementoState{s.type,		s.data,	  s.parentId, s.nodeId,	   s.referenceId, s.name,
								s.nodeType, s.region, s.pivot,	  s.keyFrames, s.palette,	  prevS.properties};
		}
	}
	return _states[0];
}

MementoState MementoHandler::undoKeyFrames(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (prevS.keyFrames.hasValue() && prevS.nodeId == s.nodeId) {
			return MementoState{s.type,		s.data,	  s.parentId,  s.nodeId,		s.referenceId, s.name,
								s.nodeType, s.region, prevS.pivot, prevS.keyFrames, s.palette,	   s.properties};
		}
	}
	return _states[0];
}

MementoState MementoHandler::undoRename(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (!prevS.name.empty() && prevS.nodeId == s.nodeId) {
			return MementoState{s.type,		s.data,	  s.parentId, s.nodeId,		 s.referenceId, prevS.name,
								s.nodeType, s.region, s.pivot,	  s.worldMatrix, s.keyFrameIdx, s.palette};
		}
	}
	return _states[0];
}

MementoState MementoHandler::undoMove(const MementoState &s) {
	for (int i = _statePosition; i >= 0; --i) {
		MementoState &prevS = _states[i];
		if (prevS.parentId != InvalidNodeId && prevS.nodeId == s.nodeId) {
			return MementoState{s.type,		s.data,	  prevS.parentId, s.nodeId,		 s.referenceId, prevS.name,
								s.nodeType, s.region, s.pivot,		  s.worldMatrix, s.keyFrameIdx, s.palette};
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
	} else if (s.type == MementoType::SceneNodeMove) {
		return undoMove(s);
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

void MementoHandler::markNodePropertyChange(const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	if (!markUndoPreamble(nodeId)) {
		return;
	}
	const int parentId = node.parent();
	const core::String &name = node.name();
	Log::debug("New node property undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(), (int)_states.size());
	core::Optional<scenegraph::SceneGraphNodeProperties> properties;
	properties.setValue(node.properties());
	MementoState state(MementoType::SceneNodeProperties, {}, parentId, nodeId, node.reference(), name, node.type(),
					   voxel::Region::InvalidRegion, node.pivot(), {}, {}, properties);
	addState(core::move(state));
}

void MementoHandler::markKeyFramesChange(const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	voxel::RawVolume *volume = nullptr;
	Log::debug("Mark node %i key frame changes (%s)", nodeId, name.c_str());
	markUndoKeyFrames(parentId, nodeId, node.reference(), name, node.type(), volume, MementoType::SceneNodeKeyFrames, voxel::Region::InvalidRegion, node.pivot(), node.allKeyFrames());
}

void MementoHandler::markNodeRemoved(const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	const voxel::RawVolume *volume = node.volume();
	Log::debug("Mark node %i as deleted (%s)", nodeId, name.c_str());
	core::Optional<palette::Palette> palette;
	palette.setValue(node.palette());
	Log::debug("palette node added hash: %" PRIu64, node.palette().hash());
	markUndoKeyFrames(parentId, nodeId, node.reference(), name, node.type(), volume, MementoType::SceneNodeRemoved,
					  voxel::Region::InvalidRegion, node.pivot(), node.allKeyFrames(), palette);
}

void MementoHandler::markNodeAdded(const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	const voxel::RawVolume *volume = node.volume();
	Log::debug("Mark node %i as added (%s)", nodeId, name.c_str());
	core::Optional<palette::Palette> palette;
	palette.setValue(node.palette());
	Log::debug("palette node added hash: %" PRIu64, node.palette().hash());
	core::Optional<scenegraph::SceneGraphNodeProperties> properties;
	properties.setValue(node.properties());
	markUndoKeyFrames(parentId, nodeId, node.reference(), name, node.type(), volume, MementoType::SceneNodeAdded,
					  voxel::Region::InvalidRegion, node.pivot(), node.allKeyFrames(), palette, properties);
}

void MementoHandler::markInitialNodeState(const scenegraph::SceneGraphNode &node) {
	markNodeAdded(node);
}

void MementoHandler::markModification(const scenegraph::SceneGraphNode &node, const voxel::Region& modifiedRegion) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	const voxel::RawVolume *volume = node.volume();
	Log::debug("Mark node %i modification (%s)", nodeId, name.c_str());
	core::Optional<palette::Palette> palette;
	if (_states.empty()) {
		palette.setValue(node.palette());
		Log::debug("palette modification hash: %" PRIu64, node.palette().hash());
	}
	markUndo(parentId, nodeId, node.reference(), name, node.type(), volume, MementoType::Modification, modifiedRegion,
			 node.pivot(), glm::mat4(1.0f), 0, palette);
}

void MementoHandler::markPaletteChange(const scenegraph::SceneGraphNode &node, const voxel::Region& modifiedRegion) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const voxel::RawVolume *volume = nullptr;
	if (modifiedRegion.isValid()) {
		volume = node.volume();
	}
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &transformMatrix = transform.worldMatrix();
	Log::debug("Mark node %i palette change (%s)", nodeId, name.c_str());
	core::Optional<palette::Palette> palette;
	palette.setValue(node.palette());
	Log::debug("palette change hash: %" PRIu64, node.palette().hash());
	markUndo(parentId, nodeId, node.reference(), name, node.type(), volume, MementoType::SceneNodePaletteChanged,
			 modifiedRegion, node.pivot(), transformMatrix, keyFrameIdx, palette);
}

void MementoHandler::markNodeRenamed(const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const voxel::RawVolume *volume = node.volume();
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &transformMatrix = transform.worldMatrix();
	Log::debug("Mark node %i renamed (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, node.reference(), name, node.type(), volume, MementoType::SceneNodeRenamed,
			 voxel::Region::InvalidRegion, node.pivot(), transformMatrix, keyFrameIdx);
}

void MementoHandler::markNodeMoved(int targetId, int sourceId) {
	markUndo(targetId, sourceId, InvalidNodeId, "", scenegraph::SceneGraphNodeType::Max, nullptr,
			 MementoType::SceneNodeMove, voxel::Region::InvalidRegion, glm::vec3{0.0f}, glm::mat4(1.0f), -1);
}

void MementoHandler::markNodeTransform(const scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &worldMatrix = transform.worldMatrix();
	Log::debug("Mark node %i as translated (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, node.reference(), name, node.type(), nullptr, MementoType::SceneNodeTransform,
			 voxel::Region::InvalidRegion, node.pivot(), worldMatrix, keyFrameIdx);
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

void MementoHandler::markUndo(int parentId, int nodeId, int referenceId, const core::String &name, scenegraph::SceneGraphNodeType nodeType, const voxel::RawVolume *volume,
							  MementoType type, const voxel::Region &region, const glm::vec3 &pivot, const glm::mat4 &worldMatrix,
							  scenegraph::KeyFrameIndex keyFrameIdx, const core::Optional<palette::Palette> &palette) {
	if (!markUndoPreamble(nodeId)) {
		return;
	}
	Log::debug("New undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(), (int)_states.size());
	voxel::logRegion("MarkUndo", region);
	const MementoData &data = MementoData::fromVolume(volume, region);
	MementoState state(type, data, parentId, nodeId, referenceId, name, nodeType, region, pivot, worldMatrix,
					   keyFrameIdx, palette);
	addState(core::move(state));
}

bool MementoHandler::removeLast() {
	if (_states.empty()) {
		return false;
	}
	_states.erase_back(1);
	return true;
}

void MementoHandler::markUndoKeyFrames(int parentId, int nodeId, int referenceId, const core::String &name, scenegraph::SceneGraphNodeType nodeType,
									   const voxel::RawVolume *volume, MementoType type, const voxel::Region &region, const glm::vec3 &pivot,
									   const scenegraph::SceneGraphKeyFramesMap &keyFrames,
									   const core::Optional<palette::Palette> &palette,
									   const core::Optional<scenegraph::SceneGraphNodeProperties> &properties) {
	if (!markUndoPreamble(nodeId)) {
		return;
	}
	Log::debug("New undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(), (int)_states.size());
	voxel::logRegion("MarkUndo", region);
	const MementoData& data = MementoData::fromVolume(volume, region);
	core::Optional<scenegraph::SceneGraphKeyFramesMap> kf;
	kf.setValue(keyFrames);
	MementoState state(type, data, parentId, nodeId, referenceId, name, nodeType, region, pivot, kf, palette,
					   properties);
	addState(core::move(state));
}

bool MementoHandler::mergeStates(MementoState &state, MementoState &merge) const {
	if (state.type == MementoType::Max) {
		state = core::move(merge);
		Log::debug("Initial memento group state is %i", (int)state.type);
		return true;
	}

	if (merge.type == MementoType::Modification) {
		if (state.type == MementoType::PaletteChanged || state.type == MementoType::SceneNodePaletteChanged) {
			Log::debug("Merge memento state of type %i into %i", (int)merge.type, (int)state.type);
			state.type = merge.type;
			state.data = core::move(merge.data);
		} else {
			Log::debug("Merge of %i into %i is not possible or not implemented yet", (int)merge.type, (int)state.type);
			return false;
		}
		core_assert(state.hasVolumeData());
	} else if (state.type != MementoType::Modification) {
		Log::debug("Merge of %i into %i is not possible or not implemented yet", (int)merge.type, (int)state.type);
		return false;
	}

	if (state.parentId == InvalidNodeId) {
		state.parentId = merge.parentId;
		Log::debug("Merged parent id");
	}
	if (state.nodeId == InvalidNodeId) {
		state.nodeId = merge.nodeId;
		Log::debug("Merged node id");
	}
	if (state.referenceId == InvalidNodeId) {
		state.referenceId = merge.referenceId;
		Log::debug("Merged reference id");
	}
	if (state.nodeType == scenegraph::SceneGraphNodeType::Max) {
		state.nodeType = merge.nodeType;
		Log::debug("Merged node type");
	}
	if (state.keyFrameIdx == InvalidKeyFrame) {
		state.keyFrameIdx = merge.keyFrameIdx;
		Log::debug("Merged key frame index");
	}
	if (!state.region.isValid()) {
		state.region = merge.region;
		Log::debug("Merged region");
	}
	if (merge.palette.hasValue() && !state.palette.hasValue()) {
		state.palette = merge.palette;
		Log::debug("Merged palette");
	}
	if (merge.properties.hasValue() && !state.properties.hasValue()) {
		state.properties = merge.properties;
		Log::debug("Merged properties");
	}
	if (merge.keyFrames.hasValue() && !state.keyFrames.hasValue()) {
		state.keyFrames = merge.keyFrames;
		Log::debug("Merged key frames");
	}
	if (!merge.name.empty() && state.name.empty()) {
		state.name = merge.name;
		Log::debug("Merged name");
	}
	if (!merge.pivot.hasValue() && state.pivot.hasValue()) {
		state.pivot = merge.pivot;
		Log::debug("Merged pivot");
	}
	if (!merge.worldMatrix.hasValue() && state.worldMatrix.hasValue()) {
		state.worldMatrix = merge.worldMatrix;
		Log::debug("Merged world matrix");
	}
	// TODO: memento group - finish implementation see https://github.com/vengi-voxel/vengi/issues/376
	return true;
}

void MementoHandler::addState(MementoState &&state) {
	if (_groupState.hasValue() && mergeStates(*_groupState.value(), state)) {
		Log::debug("Merged memento state into group");
		return;
	}
	_states.emplace_back(state);
	_statePosition = stateSize() - 1;
}

}
