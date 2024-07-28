/**
 * @file
 */

#include "MementoHandler.h"

#include "command/Command.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VoxelUtil.h"
#include <inttypes.h>

namespace memento {

static const MementoStateGroup InvalidMementoGroup{};

MementoData::MementoData(uint8_t *buf, size_t bufSize, const voxel::Region &region)
	: _compressedSize(bufSize), _region(region) {
	if (buf != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = buf;
	} else {
		core_assert(_compressedSize == 0);
	}
}

MementoData::MementoData(const uint8_t *buf, size_t bufSize, const voxel::Region &region)
	: _compressedSize(bufSize), _region(region) {
	if (buf != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = (uint8_t *)core_malloc(_compressedSize);
		core_memcpy(_buffer, buf, _compressedSize);
	} else {
		core_assert(_compressedSize == 0);
	}
}

MementoData::MementoData(MementoData &&o) noexcept
	: _compressedSize(o._compressedSize), _buffer(o._buffer), _region(o._region) {
	o._compressedSize = 0;
	o._buffer = nullptr;
}

MementoData::~MementoData() {
	if (_buffer != nullptr) {
		core_free(_buffer);
		_buffer = nullptr;
	}
}

MementoData::MementoData(const MementoData &o) : _compressedSize(o._compressedSize), _region(o._region) {
	if (o._buffer != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = (uint8_t *)core_malloc(_compressedSize);
		core_memcpy(_buffer, o._buffer, _compressedSize);
	} else {
		core_assert(_compressedSize == 0);
	}
}

MementoData &MementoData::operator=(MementoData &&o) noexcept {
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

MementoData MementoData::fromVolume(const voxel::RawVolume *volume, const voxel::Region &region) {
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

bool MementoData::toVolume(voxel::RawVolume *volume, const MementoData &mementoData) {
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
	uint8_t *uncompressedBuf = (uint8_t *)core_malloc(uncompressedBufferSize);
	if (stream.read(uncompressedBuf, uncompressedBufferSize) == -1) {
		core_free(uncompressedBuf);
		return false;
	}
	core::ScopedPtr<voxel::RawVolume> v(
		voxel::RawVolume::createRaw((voxel::Voxel *)uncompressedBuf, mementoData.region()));
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

void MementoHandler::beginGroup(const core::String &name) {
	if (_locked > 0) {
		Log::debug("Don't add undo group state - we are currently in locked mode");
		return;
	}

	Log::debug("Begin memento group: %i (%s)", _groupState, name.c_str());
	if (_groupState <= 0) {
		_groups.emplace_back(MementoStateGroup{name, {}});
		_groupStatePosition = stateSize() - 1;
	}
	++_groupState;
}

void MementoHandler::endGroup() {
	if (_locked > 0) {
		core_assert(_groupState <= 0);
		Log::debug("Don't add undo group state - we are currently in locked mode");
		return;
	}
	Log::debug("End memento group: %i", _groupState);
	core_assert(_groupState > 0);
	--_groupState;
	if (_groupState <= 0) {
		core_assert(!_groups.empty());
		if (_groups.back().states.empty()) {
			removeLast();
		}
	}
}

const char *MementoHandler::typeToString(MementoType type) {
	const char *states[] = {
		"Modification",		   "SceneNodeMove",		 "SceneNodeAdded",			"SceneNodeRemoved",
		"SceneNodeRenamed",	   "SceneNodeTransform", "SceneNodePaletteChanged", "SceneNodeKeyFrames",
		"SceneNodeProperties", "PaletteChanged"};
	static_assert((int)MementoType::Max == lengthof(states), "Array sizes don't match");
	return states[(int)type];
}

void MementoHandler::printState(const MementoState &state) const {
	const glm::ivec3 &mins = state.region.getLowerCorner();
	const glm::ivec3 &maxs = state.region.getUpperCorner();
	core::String palHash;
	if (state.palette.hasValue()) {
		palHash = core::string::toString(state.palette.value()->hash());
	}
	Log::info("%s: node id: %i", typeToString(state.type), state.nodeId);
	Log::info(" - parent: %i", state.parentId);
	Log::info(" - key frame index: %i", state.keyFrameIdx);
	Log::info(" - name: %s", state.name.c_str());
	Log::info(" - localMatrix");
	if (state.localMatrix.hasValue()) {
		const glm::mat4 &m = *state.localMatrix.value();
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
				const glm::mat4 &m = f.transform().localMatrix();
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
	Log::info("Current memento state index: %i", _groupStatePosition);

	for (const MementoStateGroup &group : _groups) {
		Log::info("Group: %s", group.name.c_str());
		for (const MementoState &state : group.states) {
			printState(state);
		}
	}
}

void MementoHandler::construct() {
	command::Command::registerCommand("ve_mementoinfo", [&](const command::CmdArgs &args) { print(); });
}

void MementoHandler::clearStates() {
	core_assert_msg(_groupState <= 0, "You should not clear the states while you are recording a group state");
	_groups.clear();
	_groupStatePosition = 0u;
}

MementoState MementoHandler::undoModification(const MementoState &s) {
	core_assert(s.hasVolumeData());
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.nodeId != s.nodeId) {
				continue;
			}
			if (prevS.type == MementoType::Modification || prevS.type == MementoType::SceneNodeAdded) {
				core_assert(prevS.hasVolumeData() || prevS.referenceId != InvalidNodeId);
				voxel::logRegion("Undo current", s.region);
				voxel::logRegion("Undo previous", prevS.region);
				voxel::logRegion("Undo current data", s.data.region());
				voxel::logRegion("Undo previous data", prevS.data.region());
				// use the region from the current state - but the volume and palette from the previous state of this
				// node
				return MementoState{s.type,		   prevS.data,	   s.parentId, s.nodeId, prevS.referenceId,
									s.name,		   prevS.nodeType, s.region,   s.pivot,	 s.localMatrix,
									s.keyFrameIdx, s.palette};
			}
		}
	}

	core_assert_msg(
		first(_groups[0]).type == MementoType::Modification ||
			(first(_groups[0]).referenceId != InvalidNodeId && first(_groups[0]).type == MementoType::SceneNodeAdded),
		"Expected to have a modification or scene node added with a reference state at the beginning, but got %i",
		(int)first(_groups[0]).type);
	return first(_groups[0]);
}

MementoState MementoHandler::undoTransform(const MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.nodeId != s.nodeId) {
				continue;
			}
			if ((prevS.type == MementoType::SceneNodeTransform || prevS.type == MementoType::Modification) &&
				prevS.keyFrameIdx == s.keyFrameIdx) {
				return MementoState{s.type,		s.data,	  s.parentId, s.nodeId,			 s.referenceId, s.name,
									s.nodeType, s.region, s.pivot,	  prevS.localMatrix, s.keyFrameIdx, s.palette};
			}
			if (prevS.type == MementoType::SceneNodeAdded && prevS.keyFrames.hasValue()) {
				for (const auto &e : *prevS.keyFrames.value()) {
					for (const auto &f : e->second) {
						if (f.frameIdx == s.keyFrameIdx) {
							return MementoState{
								s.type,		   s.data,	   s.parentId, s.nodeId, s.referenceId,
								s.name,		   s.nodeType, s.region,   s.pivot,	 f.transform().localMatrix(),
								s.keyFrameIdx, s.palette};
						}
					}
				}
			}
		}
	}
	return first(_groups[0]);
}

MementoState MementoHandler::undoPaletteChange(const MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.palette.hasValue() && prevS.nodeId == s.nodeId) {
				return MementoState{s.type,		s.data,	  s.parentId, s.nodeId,		 s.referenceId, s.name,
									s.nodeType, s.region, s.pivot,	  s.localMatrix, s.keyFrameIdx, prevS.palette};
			}
		}
	}
	return first(_groups[0]);
}

MementoState MementoHandler::undoNodeProperties(const MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.properties.hasValue() && prevS.nodeId == s.nodeId) {
				return MementoState{s.type,		s.data,	  s.parentId, s.nodeId,	   s.referenceId, s.name,
									s.nodeType, s.region, s.pivot,	  s.keyFrames, s.palette,	  prevS.properties};
			}
		}
	}
	return first(_groups[0]);
}

MementoState MementoHandler::undoKeyFrames(const MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.keyFrames.hasValue() && prevS.nodeId == s.nodeId) {
				return MementoState{s.type,		s.data,	  s.parentId,  s.nodeId,		s.referenceId, s.name,
									s.nodeType, s.region, prevS.pivot, prevS.keyFrames, s.palette,	   s.properties};
			}
		}
	}
	return first(_groups[0]);
}

MementoState MementoHandler::undoRename(const MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (!prevS.name.empty() && prevS.nodeId == s.nodeId) {
				return MementoState{s.type,		s.data,	  s.parentId, s.nodeId,		 s.referenceId, prevS.name,
									s.nodeType, s.region, s.pivot,	  s.localMatrix, s.keyFrameIdx, s.palette};
			}
		}
	}
	return first(_groups[0]);
}

MementoState MementoHandler::undoMove(const MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.parentId != InvalidNodeId && prevS.nodeId == s.nodeId) {
				return MementoState{s.type,		s.data,	  prevS.parentId, s.nodeId,		 s.referenceId, prevS.name,
									s.nodeType, s.region, s.pivot,		  s.localMatrix, s.keyFrameIdx, s.palette};
			}
		}
	}
	return first(_groups[0]);
}

MementoStateGroup MementoHandler::undo() {
	if (!canUndo()) {
		return InvalidMementoGroup;
	}
	Log::debug("Available states: %i, current index: %i", (int)_groups.size(), _groupStatePosition);
	const MementoStateGroup &group = stateGroup();
	core_assert(!group.states.empty());
	--_groupStatePosition;
	MementoStateGroup targetGroup;
	targetGroup.states.reserve(group.states.size());
	Log::debug("Undo group states: %i", (int)group.states.size());
	for (const MementoState &s : group.states) {
		Log::debug("Undo memento type %s", typeToString(s.type));
		if (s.type == MementoType::Modification) {
			targetGroup.states.emplace_back(undoModification(s));
		} else if (s.type == MementoType::SceneNodeTransform) {
			targetGroup.states.emplace_back(undoTransform(s));
		} else if (s.type == MementoType::SceneNodePaletteChanged) {
			targetGroup.states.emplace_back(undoPaletteChange(s));
		} else if (s.type == MementoType::SceneNodeProperties) {
			targetGroup.states.emplace_back(undoNodeProperties(s));
		} else if (s.type == MementoType::SceneNodeKeyFrames) {
			targetGroup.states.emplace_back(undoKeyFrames(s));
		} else if (s.type == MementoType::SceneNodeRenamed) {
			targetGroup.states.emplace_back(undoRename(s));
		} else if (s.type == MementoType::SceneNodeMove) {
			targetGroup.states.emplace_back(undoMove(s));
		} else {
			targetGroup.states.emplace_back(s);
		}
	}
	core_assert(!targetGroup.states.empty());
	return targetGroup;
}

MementoStateGroup MementoHandler::redo() {
	if (!canRedo()) {
		return InvalidMementoGroup;
	}
	++_groupStatePosition;
	Log::debug("Available states: %i, current index: %i", (int)_groups.size(), _groupStatePosition);
	return stateGroup();
}

void MementoHandler::updateNodeId(int nodeId, int newNodeId) {
	for (MementoStateGroup &group : _groups) {
		for (MementoState &state : group.states) {
			if (state.nodeId == nodeId) {
				state.nodeId = newNodeId;
			}
			if (state.parentId == nodeId) {
				state.parentId = newNodeId;
			}
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
	Log::debug("New node property undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(),
			   (int)_groups.size());
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
	markUndoKeyFrames(parentId, nodeId, node.reference(), name, node.type(), volume, MementoType::SceneNodeKeyFrames,
					  voxel::Region::InvalidRegion, node.pivot(), node.allKeyFrames());
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

void MementoHandler::markModification(const scenegraph::SceneGraphNode &node, const voxel::Region &modifiedRegion) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	const voxel::RawVolume *volume = node.volume();
	Log::debug("Mark node %i modification (%s)", nodeId, name.c_str());
	core::Optional<palette::Palette> palette;
	if (_groups.empty()) {
		palette.setValue(node.palette());
		Log::debug("palette modification hash: %" PRIu64, node.palette().hash());
	}
	markUndo(parentId, nodeId, node.reference(), name, node.type(), volume, MementoType::Modification, modifiedRegion,
			 node.pivot(), glm::mat4(1.0f), 0, palette);
}

void MementoHandler::markPaletteChange(const scenegraph::SceneGraphNode &node, const voxel::Region &modifiedRegion) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const voxel::RawVolume *volume = nullptr;
	if (modifiedRegion.isValid()) {
		volume = node.volume();
	}
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &localMatrix = transform.localMatrix();
	Log::debug("Mark node %i palette change (%s)", nodeId, name.c_str());
	core::Optional<palette::Palette> palette;
	palette.setValue(node.palette());
	Log::debug("palette change hash: %" PRIu64, node.palette().hash());
	markUndo(parentId, nodeId, node.reference(), name, node.type(), volume, MementoType::SceneNodePaletteChanged,
			 modifiedRegion, node.pivot(), localMatrix, keyFrameIdx, palette);
}

void MementoHandler::markNodeRenamed(const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const int parentId = node.parent();
	const core::String &name = node.name();
	scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const voxel::RawVolume *volume = node.volume();
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::mat4 &localMatrix = transform.localMatrix();
	Log::debug("Mark node %i renamed (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, node.reference(), name, node.type(), volume, MementoType::SceneNodeRenamed,
			 voxel::Region::InvalidRegion, node.pivot(), localMatrix, keyFrameIdx);
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
	const glm::mat4 &localMatrix = transform.localMatrix();
	Log::debug("Mark node %i as translated (%s)", nodeId, name.c_str());
	markUndo(parentId, nodeId, node.reference(), name, node.type(), nullptr, MementoType::SceneNodeTransform,
			 voxel::Region::InvalidRegion, node.pivot(), localMatrix, keyFrameIdx);
}

void MementoHandler::markAddedAnimation(const core::String &animation) {
	// TODO: MEMENTO: implement me
}

bool MementoHandler::markUndoPreamble(int nodeId) {
	if (_locked > 0) {
		Log::debug("Don't add undo state - we are currently in locked mode");
		return false;
	}
	core_assert(nodeId >= 0);
	if (!_groups.empty()) {
		// if we mark something as new undo state, we can throw away
		// every other state that follows the new one (everything after
		// the current state position)
		const size_t n = _groups.size() - (_groupStatePosition + 1);
		_groups.erase_back(n);
	}
	return true;
}

void MementoHandler::markUndo(int parentId, int nodeId, int referenceId, const core::String &name,
							  scenegraph::SceneGraphNodeType nodeType, const voxel::RawVolume *volume, MementoType type,
							  const voxel::Region &region, const glm::vec3 &pivot, const glm::mat4 &localMatrix,
							  scenegraph::KeyFrameIndex keyFrameIdx, const core::Optional<palette::Palette> &palette) {
	if (!markUndoPreamble(nodeId)) {
		return;
	}
	Log::debug("New undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(),
			   (int)_groups.size());
	voxel::logRegion("MarkUndo", region);
	const MementoData &data = MementoData::fromVolume(volume, region);
	MementoState state(type, data, parentId, nodeId, referenceId, name, nodeType, region, pivot, localMatrix,
					   keyFrameIdx, palette);
	addState(core::move(state));
}

bool MementoHandler::removeLast() {
	if (_groups.empty()) {
		return false;
	}
	if (_groupStatePosition == stateSize() - 1) {
		--_groupStatePosition;
	}
	_groups.erase_back(1);
	return true;
}

void MementoHandler::markUndoKeyFrames(int parentId, int nodeId, int referenceId, const core::String &name,
									   scenegraph::SceneGraphNodeType nodeType, const voxel::RawVolume *volume,
									   MementoType type, const voxel::Region &region, const glm::vec3 &pivot,
									   const scenegraph::SceneGraphKeyFramesMap &keyFrames,
									   const core::Optional<palette::Palette> &palette,
									   const core::Optional<scenegraph::SceneGraphNodeProperties> &properties) {
	if (!markUndoPreamble(nodeId)) {
		return;
	}
	Log::debug("New undo state for node %i with name %s (memento state index: %i)", nodeId, name.c_str(),
			   (int)_groups.size());
	voxel::logRegion("MarkUndo", region);
	const MementoData &data = MementoData::fromVolume(volume, region);
	core::Optional<scenegraph::SceneGraphKeyFramesMap> kf;
	kf.setValue(keyFrames);
	MementoState state(type, data, parentId, nodeId, referenceId, name, nodeType, region, pivot, kf, palette,
					   properties);
	addState(core::move(state));
}

void MementoHandler::addState(MementoState &&state) {
	if (_groupState > 0) {
		Log::debug("add group state: %i", _groupState);
		_groups.back().states.emplace_back(state);
		return;
	}
	MementoStateGroup group;
	group.name = "single";
	group.states.emplace_back(state);
	_groups.emplace_back(core::move(group));
	_groupStatePosition = stateSize() - 1;
}

} // namespace memento
