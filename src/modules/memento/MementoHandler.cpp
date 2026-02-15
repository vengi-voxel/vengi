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
#include "core/concurrent/Lock.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipWriteStream.h"
#include "palette/NormalPalette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/VolumeCompression.h"
#include "voxel/Voxel.h"
#include "voxelutil/VoxelUtil.h"
#include <inttypes.h>

namespace memento {

static const MementoStateGroup InvalidMementoGroup{};

MementoState::MementoState() : type(MementoType::Max), nodeType(scenegraph::SceneGraphNodeType::Max) {
}

MementoState::MementoState(const MementoState &other)
	: type(other.type), data(other.data), parentUUID(other.parentUUID), nodeUUID(other.nodeUUID),
	  referenceUUID(other.referenceUUID), nodeType(other.nodeType), keyFrames(other.keyFrames),
	  properties(other.properties), name(other.name), pivot(other.pivot), palette(other.palette),
	  normalPalette(other.normalPalette), stringList(other.stringList), ikConstraint(other.ikConstraint) {
}

MementoState::MementoState(MementoType _type, const MementoState &other)
	: type(_type), data(other.data), parentUUID(other.parentUUID), nodeUUID(other.nodeUUID),
	  referenceUUID(other.referenceUUID), nodeType(other.nodeType), keyFrames(other.keyFrames),
	  properties(other.properties), name(other.name), pivot(other.pivot), palette(other.palette),
	  normalPalette(other.normalPalette), stringList(other.stringList), ikConstraint(other.ikConstraint) {
}

MementoState::MementoState(MementoState &&other) noexcept {
	type = other.type;
	data = core::move(other.data);
	parentUUID = core::move(other.parentUUID);
	nodeUUID = core::move(other.nodeUUID);
	referenceUUID = core::move(other.referenceUUID);
	nodeType = other.nodeType;
	keyFrames = core::move(other.keyFrames);
	properties = core::move(other.properties);
	name = core::move(other.name);
	pivot = core::move(other.pivot);
	palette = core::move(other.palette);
	normalPalette = core::move(other.normalPalette);
	stringList = core::move(other.stringList);
	ikConstraint = core::move(other.ikConstraint);
}

MementoState &MementoState::operator=(MementoState &&other) noexcept {
	if (&other == this) {
		return *this;
	}
	type = other.type;
	data = core::move(other.data);
	parentUUID = core::move(other.parentUUID);
	nodeUUID = core::move(other.nodeUUID);
	referenceUUID = core::move(other.referenceUUID);
	nodeType = other.nodeType;
	keyFrames = core::move(other.keyFrames);
	properties = core::move(other.properties);
	name = core::move(other.name);
	pivot = core::move(other.pivot);
	palette = core::move(other.palette);
	normalPalette = core::move(other.normalPalette);
	stringList = core::move(other.stringList);
	ikConstraint = core::move(other.ikConstraint);
	return *this;
}

MementoState &MementoState::operator=(const MementoState &other) {
	if (&other == this) {
		return *this;
	}
	type = other.type;
	data = other.data;
	parentUUID = other.parentUUID;
	nodeUUID = other.nodeUUID;
	referenceUUID = other.referenceUUID;
	nodeType = other.nodeType;
	keyFrames = other.keyFrames;
	properties = other.properties;
	name = other.name;
	pivot = other.pivot;
	palette = other.palette;
	normalPalette = other.normalPalette;
	stringList = other.stringList;
	ikConstraint = other.ikConstraint;
	return *this;
}

MementoState::MementoState(MementoType _type, const MementoData &_data, const core::UUID &_parentId,
						   const core::UUID &_nodeId, const core::UUID &_referenceId, const core::String &_name,
						   scenegraph::SceneGraphNodeType _nodeType, const glm::vec3 &_pivot,
						   const scenegraph::SceneGraphKeyFramesMap &_keyFrames, const palette::Palette &_palette,
						   const palette::NormalPalette &_normalPalette,
						   const scenegraph::SceneGraphNodeProperties &_properties)
	: type(_type), data(_data), parentUUID(_parentId), nodeUUID(_nodeId), referenceUUID(_referenceId),
	  nodeType(_nodeType), keyFrames(_keyFrames), properties(_properties), name(_name), pivot(_pivot),
	  palette(_palette), normalPalette(_normalPalette) {
}

MementoState::MementoState(MementoType _type, MementoData &&_data, core::UUID &&_parentId, core::UUID &&_nodeId,
						   core::UUID &&_referenceId, core::String &&_name, scenegraph::SceneGraphNodeType _nodeType,
						   glm::vec3 &&_pivot, scenegraph::SceneGraphKeyFramesMap &&_keyFrames,
						   palette::Palette &&_palette, palette::NormalPalette &&_normalPalette,
						   scenegraph::SceneGraphNodeProperties &&_properties)
	: type(_type), data(_data), parentUUID(_parentId), nodeUUID(_nodeId), referenceUUID(_referenceId),
	  nodeType(_nodeType), keyFrames(_keyFrames), properties(_properties), name(_name), pivot(_pivot),
	  palette(_palette), normalPalette(_normalPalette) {
}

MementoState::MementoState(MementoType _type, const core::DynamicArray<core::String> &_stringList)
	: type(_type), nodeType(scenegraph::SceneGraphNodeType::Max), pivot(0.0f), stringList(_stringList) {
}

MementoData::MementoData(uint8_t *buf, size_t bufSize, const voxel::Region &dataRegion,
						 const voxel::Region &volumeRegion)
	: _compressedSize(bufSize), _dataRegion(dataRegion), _volumeRegion(volumeRegion), _modifiedRegion(dataRegion) {
	if (buf != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = buf;
	} else {
		core_assert(_compressedSize == 0);
	}
}

MementoData::MementoData(const uint8_t *buf, size_t bufSize, const voxel::Region &dataRegion,
						 const voxel::Region &volumeRegion)
	: _compressedSize(bufSize), _dataRegion(dataRegion), _volumeRegion(volumeRegion), _modifiedRegion(dataRegion) {
	if (buf != nullptr) {
		core_assert(_compressedSize > 0);
		_buffer = (uint8_t *)core_malloc(_compressedSize);
		core_memcpy(_buffer, buf, _compressedSize);
	} else {
		core_assert(_compressedSize == 0);
	}
}

MementoData::MementoData(MementoData &&o) noexcept
	: _compressedSize(o._compressedSize), _buffer(o._buffer), _dataRegion(o._dataRegion),
	  _volumeRegion(o._volumeRegion), _modifiedRegion(o._modifiedRegion) {
	o._compressedSize = 0;
	o._buffer = nullptr;
}

MementoData::~MementoData() {
	if (_buffer != nullptr) {
		core_free(_buffer);
		_buffer = nullptr;
	}
}

MementoData::MementoData(const MementoData &o)
	: _compressedSize(o._compressedSize), _dataRegion(o._dataRegion), _volumeRegion(o._volumeRegion),
	  _modifiedRegion(o._modifiedRegion) {
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
		_dataRegion = o._dataRegion;
		_volumeRegion = o._volumeRegion;
		_modifiedRegion = o._modifiedRegion;
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
		_dataRegion = o._dataRegion;
		_volumeRegion = o._volumeRegion;
		_modifiedRegion = o._modifiedRegion;
	}
	return *this;
}

MementoData MementoData::fromVolume(const voxel::RawVolume *volume, const voxel::Region &region) {
	if (volume == nullptr) {
		return MementoData();
	}
	// Preserve the requested region. If it's invalid, fall back to the full volume region.
	if (region.isValid()) {
		// Use the RawVolume copy-with-region constructor which will handle regions
		// that extend outside the source by filling with air or cropping as needed.
		voxel::RawVolume v(*volume, region);
		const int actualVoxels = v.region().voxels();
		io::BufferedReadWriteStream outStream((int64_t)actualVoxels * sizeof(voxel::Voxel));
		io::ZipWriteStream stream(outStream);
		if (stream.write(v.data(), actualVoxels * sizeof(voxel::Voxel)) == -1) {
			Log::error("Failed to compress memento volume data");
			return MementoData();
		}
		stream.flush();
		const size_t size = (size_t)outStream.size();
		const voxel::Region actualRegion = v.region();
		return {outStream.release(), size, actualRegion, volume->region()};
	}
	const int allVoxels = volume->region().voxels();
	io::BufferedReadWriteStream outStream((int64_t)allVoxels * sizeof(voxel::Voxel));
	io::ZipWriteStream stream(outStream);
	if (stream.write(volume->data(), allVoxels * sizeof(voxel::Voxel)) == -1) {
		Log::error("Failed to compress memento volume data");
		return MementoData();
	}
	stream.flush();
	const size_t size = (size_t)outStream.size();
	return {outStream.release(), size, volume->region(), volume->region()};
}

bool MementoData::toVolume(voxel::RawVolume *volume, const MementoData &mementoData, const voxel::Region &region) {
	if (mementoData._buffer == nullptr) {
		return false;
	}
	core_assert_always(volume != nullptr);
	if (volume == nullptr) {
		return false;
	}
	if (!voxel::intersects(mementoData.dataRegion(), region)) {
		return false;
	}

	core::ScopedPtr<voxel::RawVolume> v(
		voxel::toVolume(mementoData._buffer, (uint32_t)mementoData._compressedSize, mementoData.dataRegion()));
	if (!v) {
		return false;
	}
	// Crop the region to the data region to handle partial overlaps.
	// This is important for mirror operations where the modified region spans
	// both sides of the mirror plane but previous states may only cover a
	// partial region.
	voxel::Region copyRegion = region;
	copyRegion.cropTo(mementoData.dataRegion());
	if (!volume->copyInto(*v, copyRegion)) {
		Log::error("Failed to copy memento volume region into target volume");
	}
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
	_groupState = 0;
	clearStates();
	_listeners.clear();
}

void MementoHandler::registerListener(IMementoStateListener *listener) {
	if (listener != nullptr) {
		// Check if already added
		for (const auto *existing : _listeners) {
			if (existing == listener) {
				return;
			}
		}
		_listeners.push_back(listener);
	}
}

void MementoHandler::unregisterListener(IMementoStateListener *listener) {
	for (auto it = _listeners.begin(); it != _listeners.end(); ++it) {
		if (*it == listener) {
			_listeners.erase(it);
			break;
		}
	}
}

bool MementoHandler::canUndo() const {
	if (_locked > 0) {
		return false;
	}
	if (_groupState > 0) {
		return false;
	}
	if (stateSize() <= 1) {
		return false;
	}
	return _groupStatePosition > 0;
}

bool MementoHandler::canRedo() const {
	if (_locked > 0) {
		return false;
	}
	if (_groupState > 0) {
		return false;
	}
	if (stateSize() <= 1) {
		return false;
	}
	return _groupStatePosition <= stateSize() - 2;
}

void MementoHandler::beginGroup(const core::String &name) {
	if (_locked > 0) {
		Log::debug("Don't add undo group state - we are currently in locked mode");
		return;
	}

	Log::debug("Begin memento group: %i (%s)", _groupState, name.c_str());
	if (_groupState <= 0) {
		cutFromGroupStatePosition();
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

void MementoHandler::extractVolumeRegion(voxel::RawVolume *targetVolume, const MementoState &state) const {
	if (!state.hasVolumeData() || targetVolume == nullptr) {
		return;
	}

	voxel::Region modifiedRegion = state.data.modifiedRegion();
	if (!state.data.volumeRegion().containsRegion(modifiedRegion)) {
		modifiedRegion = state.data.dataRegion();
	}
	Log::debug("Undo region changes at %i:%i:%i - %i:%i:%i", modifiedRegion.getLowerX(),
			   modifiedRegion.getLowerY(), modifiedRegion.getLowerZ(),
			   modifiedRegion.getUpperX(), modifiedRegion.getUpperY(),
			   modifiedRegion.getUpperZ());

	// we need to walk all states because the memento data might be a partial region only
	for (int groupStatePos = 0; groupStatePos < _groupStatePosition; ++groupStatePos) {
		const MementoStateGroup &group = _groups[groupStatePos];
		for (const MementoState &s : group.states) {
			if (s.type != MementoType::Modification && s.type != MementoType::SceneNodeAdded) {
				continue;
			}
			if (s.nodeUUID != state.nodeUUID) {
				continue;
			}
			if (!memento::MementoData::toVolume(targetVolume, s.data, modifiedRegion)) {
				const core::String &uuidStr = s.nodeUUID.str();
				Log::debug("Failed to apply memento state of type %s for node %s", typeToString(s.type),
						   uuidStr.c_str());
			}
		}
	}
	if (!memento::MementoData::toVolume(targetVolume, state.data, modifiedRegion)) {
		const core::String &uuidStr = state.nodeUUID.str();
		Log::debug("Failed to apply memento state of type %s for node %s", typeToString(state.type),
				   uuidStr.c_str());
	}
}

const char *MementoHandler::typeToString(MementoType type) {
	const char *states[] = {"Modification",
							"SceneNodeMove",
							"SceneNodeAdded",
							"SceneNodeRemoved",
							"SceneNodeRenamed",
							"SceneNodePaletteChanged",
							"SceneNodeNormalPaletteChanged",
							"SceneNodeKeyFrames",
							"SceneNodeProperties",
							"SceneNodeIKConstraint",
							"SceneGraphAnimation"};
	static_assert((int)MementoType::Max == lengthof(states), "Array sizes don't match");
	return states[(int)type];
}

void MementoHandler::printState(const MementoState &state) const {
	core::String palHash = core::string::toString(state.palette.hash());
	core::String normalPalHash = core::string::toString(state.normalPalette.hash());
	const core::String &uuidStr = state.nodeUUID.str();
	Log::info("%s: node id: %s", typeToString(state.type), uuidStr.c_str());
	const core::String &parentUUIDStr = state.parentUUID.str();
	Log::info(" - parent: %s", parentUUIDStr.c_str());
	Log::info(" - name: %s", state.name.c_str());
	Log::info(" - volume: %s", state.data._buffer == nullptr ? "empty" : "volume");
	const glm::ivec3 &dataMins = state.dataRegion().getLowerCorner();
	const glm::ivec3 &dataMaxs = state.dataRegion().getUpperCorner();
	Log::info(" - dataregion: mins(%i:%i:%i)/maxs(%i:%i:%i)", dataMins.x, dataMins.y, dataMins.z, dataMaxs.x,
			  dataMaxs.y, dataMaxs.z);
	const glm::ivec3 &volumeMins = state.volumeRegion().getLowerCorner();
	const glm::ivec3 &volumeMaxs = state.volumeRegion().getUpperCorner();
	Log::info(" - volumeregion: mins(%i:%i:%i)/maxs(%i:%i:%i)", volumeMins.x, volumeMins.y, volumeMins.z, volumeMaxs.x,
			  volumeMaxs.y, volumeMaxs.z);
	Log::info(" - size: %ib", (int)state.data.size());
	Log::info(" - palette: %s", palHash.c_str());
	Log::info(" - normalPalette: %s", normalPalHash.c_str());
	Log::info(" - pivot: %f:%f:%f", state.pivot.x, state.pivot.y, state.pivot.z);
	const scenegraph::SceneGraphKeyFramesMap &keyFrames = state.keyFrames;
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
	if (!state.properties.empty()) {
		const scenegraph::SceneGraphNodeProperties &props = state.properties;
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
	command::Command::registerCommand("ve_mementoinfo")
		.setHandler([&](const command::CommandArgs &args) { print(); });
}

void MementoHandler::clearStates() {
	core_assert_msg(_groupState <= 0, "You should not clear the states while you are recording a group state");
	_groups.clear();
	_groupStatePosition = 0u;
}

void MementoHandler::undoModification(MementoState &s) {
	core_assert(s.hasVolumeData());
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.nodeUUID != s.nodeUUID) {
				continue;
			}
			if (prevS.type == MementoType::Modification || prevS.type == MementoType::SceneNodeAdded) {
				core_assert(prevS.hasVolumeData() || prevS.referenceUUID.isValid());
				const voxel::Region modifiedRegion = s.data.dataRegion();
				s.data = prevS.data;
				s.data.setModifiedRegion(modifiedRegion);
				// undo for un-reference node - so we have to make it a reference node again
				if (s.nodeType != prevS.nodeType) {
					core_assert(prevS.nodeType == scenegraph::SceneGraphNodeType::ModelReference);
					s.nodeType = prevS.nodeType;
					s.referenceUUID = prevS.referenceUUID;
				}
				return;
			}
		}
	}

	const core::String &uuidStr = s.nodeUUID.str();
	Log::warn("No previous modification state found for node %s", uuidStr.c_str());
}

void MementoHandler::undoPaletteChange(MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.nodeUUID == s.nodeUUID) {
				s.palette = prevS.palette;
				return;
			}
		}
	}
	const core::String &uuidStr = s.nodeUUID.str();
	Log::warn("No previous palette found for node %s", uuidStr.c_str());
}

void MementoHandler::undoNormalPaletteChange(MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.nodeUUID == s.nodeUUID) {
				s.normalPalette = prevS.normalPalette;
				return;
			}
		}
	}
	const core::String &uuidStr = s.nodeUUID.str();
	Log::warn("No previous palette found for node %s", uuidStr.c_str());
}

void MementoHandler::undoNodeProperties(MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.nodeUUID == s.nodeUUID) {
				s.properties = prevS.properties;
				return;
			}
		}
	}
	const core::String &uuidStr = s.nodeUUID.str();
	Log::warn("No previous node properties found for node %s", uuidStr.c_str());
}

void MementoHandler::undoIKConstraint(MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.nodeUUID == s.nodeUUID) {
				s.ikConstraint = prevS.ikConstraint;
				return;
			}
		}
	}
	const core::String &uuidStr = s.nodeUUID.str();
	Log::warn("No previous IK constraint found for node %s", uuidStr.c_str());
}

void MementoHandler::undoKeyFrames(MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.nodeUUID == s.nodeUUID) {
				s.keyFrames = prevS.keyFrames;
				s.pivot = prevS.pivot;
				return;
			}
		}
	}
	const core::String &uuidStr = s.nodeUUID.str();
	Log::warn("No previous node keyframes found for node %s", uuidStr.c_str());
}

void MementoHandler::undoAnimations(MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.type == MementoType::SceneGraphAnimation) {
				s.stringList = prevS.stringList;
				return;
			}
		}
	}
	Log::warn("No previous animations state found");
}

void MementoHandler::undoRename(MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.nodeUUID == s.nodeUUID) {
				s.name = prevS.name;
				return;
			}
		}
	}
	const core::String &uuidStr = s.nodeUUID.str();
	Log::warn("No previous name found for node %s", uuidStr.c_str());
}

void MementoHandler::undoMove(MementoState &s) {
	for (int i = _groupStatePosition; i >= 0; --i) {
		const MementoStateGroup &group = _groups[i];
		for (const MementoState &prevS : group.states) {
			if (prevS.parentUUID != s.parentUUID && prevS.nodeUUID == s.nodeUUID) {
				s.parentUUID = prevS.parentUUID;
				return;
			}
		}
	}
	const core::String &uuidStr = s.nodeUUID.str();
	Log::warn("No previous parent found for node %s", uuidStr.c_str());
}

MementoStateGroup MementoHandler::undo() {
	if (!canUndo()) {
		return InvalidMementoGroup;
	}
	Log::debug("Available states: %i, current index: %i", (int)_groups.size(), _groupStatePosition);
	MementoStateGroup group = stateGroup();
	core_assert(!group.states.empty());
	--_groupStatePosition;
	Log::debug("Undo group states: %i", (int)group.states.size());
	for (MementoState &s : group.states) {
		Log::debug("Undo memento type %s", typeToString(s.type));
		if (s.type == MementoType::Modification) {
			undoModification(s);
		} else if (s.type == MementoType::SceneNodePaletteChanged) {
			undoPaletteChange(s);
		} else if (s.type == MementoType::SceneNodeNormalPaletteChanged) {
			undoNormalPaletteChange(s);
		} else if (s.type == MementoType::SceneNodeProperties) {
			undoNodeProperties(s);
		} else if (s.type == MementoType::SceneNodeIKConstraint) {
			undoIKConstraint(s);
		} else if (s.type == MementoType::SceneNodeKeyFrames) {
			undoKeyFrames(s);
		} else if (s.type == MementoType::SceneGraphAnimation) {
			undoAnimations(s);
		} else if (s.type == MementoType::SceneNodeRenamed) {
			undoRename(s);
		} else if (s.type == MementoType::SceneNodeMove) {
			undoMove(s);
		}
	}
	return group;
}

MementoStateGroup MementoHandler::redo() {
	if (!canRedo()) {
		return InvalidMementoGroup;
	}
	++_groupStatePosition;
	Log::debug("Available states: %i, current index: %i", (int)_groups.size(), _groupStatePosition);
	return stateGroup();
}

bool MementoHandler::markAllAnimations(const core::DynamicArray<core::String> &animations) {
	Log::debug("Add all (%i) animations from the scenegraph to the memento state", (int)animations.size());
	MementoState state(MementoType::SceneGraphAnimation, animations);
	addState(core::move(state));
	return true;
}

bool MementoHandler::markNodePropertyChange(const scenegraph::SceneGraph &sceneGraph,
											const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const core::String &name = node.name();
	voxel::RawVolume *volume = nullptr;
	Log::debug("New node property memento state for node %i with name %s", nodeId, name.c_str());
	return markUndo(sceneGraph, node, volume, MementoType::SceneNodeProperties, voxel::Region::InvalidRegion);
}

bool MementoHandler::markIKConstraintChange(const scenegraph::SceneGraph &sceneGraph,
											const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const core::String &name = node.name();
	voxel::RawVolume *volume = nullptr;
	Log::debug("New IK constraint memento state for node %i with name %s", nodeId, name.c_str());
	return markUndo(sceneGraph, node, volume, MementoType::SceneNodeIKConstraint, voxel::Region::InvalidRegion);
}

bool MementoHandler::markKeyFramesChange(const scenegraph::SceneGraph &sceneGraph,
										 const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const core::String &name = node.name();
	voxel::RawVolume *volume = nullptr;
	Log::debug("Mark node %i key frame changes (%s)", nodeId, name.c_str());
	return markUndo(sceneGraph, node, volume, MementoType::SceneNodeKeyFrames, voxel::Region::InvalidRegion);
}

bool MementoHandler::markNodeRemove(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const core::String &name = node.name();
	const voxel::RawVolume *volume = node.volume();
	Log::debug("Mark node %i as deleted (%s)", nodeId, name.c_str());
	return markUndo(sceneGraph, node, volume, MementoType::SceneNodeRemoved, voxel::Region::InvalidRegion);
}

bool MementoHandler::markNodeAdded(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const core::String &name = node.name();
	const voxel::RawVolume *volume = node.volume();
	Log::debug("Mark node %i as added (%s)", nodeId, name.c_str());
	return markUndo(sceneGraph, node, volume, MementoType::SceneNodeAdded, voxel::Region::InvalidRegion);
}

bool MementoHandler::markInitialSceneState(const scenegraph::SceneGraph &sceneGraph) {
	memento::ScopedMementoGroup mementoGroup(*this, "initialscene");
	if (!markAllAnimations(sceneGraph.animations())) {
		return false;
	}
	sceneGraph.nodes().for_parallel(
		[&](const scenegraph::SceneGraphNodes::key_type &n, const scenegraph::SceneGraphNodes::value_type &v) {
			markInitialNodeState(sceneGraph, v);
		});
	return true;
}

bool MementoHandler::markInitialNodeState(const scenegraph::SceneGraph &sceneGraph,
										  const scenegraph::SceneGraphNode &node) {
	return markNodeAdded(sceneGraph, node);
}

bool MementoHandler::markModification(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
									  const voxel::Region &modifiedRegion) {
	const voxel::RawVolume *volume = node.volume();
	// Modification without volume isn't possible - so skip it here already
	if (volume == nullptr) {
		return false;
	}
	const int nodeId = node.id();
	const core::String &name = node.name();
	Log::debug("Mark node %i modification (%s)", nodeId, name.c_str());
	return markUndo(sceneGraph, node, volume, MementoType::Modification, modifiedRegion);
}

bool MementoHandler::markNormalPaletteChange(const scenegraph::SceneGraph &sceneGraph,
											 const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const core::String &name = node.name();
	const voxel::RawVolume *volume = nullptr;
	Log::debug("Mark node %i normal palette change (%s)", nodeId, name.c_str());
	return markUndo(sceneGraph, node, volume, MementoType::SceneNodeNormalPaletteChanged, voxel::Region::InvalidRegion);
}

bool MementoHandler::markPaletteChange(const scenegraph::SceneGraph &sceneGraph,
									   const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const core::String &name = node.name();
	const voxel::RawVolume *volume = nullptr;
	Log::debug("Mark node %i palette change (%s)", nodeId, name.c_str());
	return markUndo(sceneGraph, node, volume, MementoType::SceneNodePaletteChanged, voxel::Region::InvalidRegion);
}

bool MementoHandler::markNodeRenamed(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node) {
	const int nodeId = node.id();
	const core::String &name = node.name();
	const voxel::RawVolume *volume = nullptr;
	Log::debug("Mark node %i renamed (%s)", nodeId, name.c_str());
	return markUndo(sceneGraph, node, volume, MementoType::SceneNodeRenamed, voxel::Region::InvalidRegion);
}

bool MementoHandler::markNodeMoved(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node) {
	return markUndo(sceneGraph, node, nullptr, MementoType::SceneNodeMove, voxel::Region::InvalidRegion);
}

bool MementoHandler::markNodeTransform(const scenegraph::SceneGraph &sceneGraph,
									   const scenegraph::SceneGraphNode &node) {
	return markKeyFramesChange(sceneGraph, node);
}

bool MementoHandler::markAnimationAdded(const scenegraph::SceneGraph &sceneGraph, const core::String &animation) {
	ScopedMementoGroup group(*this, "Add Animation");
	markAllAnimations(sceneGraph.animations());
	for (const auto &entry : sceneGraph.nodes()) {
		if (!entry->value.isAnyModelNode()) {
			continue;
		}
		if (!markKeyFramesChange(sceneGraph, entry->value)) {
			return false;
		}
	}
	return true;
}

bool MementoHandler::markAnimationRemoved(const scenegraph::SceneGraph &sceneGraph, const core::String &animation) {
	ScopedMementoGroup group(*this, "Remove Animation");
	markAllAnimations(sceneGraph.animations());
	for (const auto &entry : sceneGraph.nodes()) {
		if (!entry->value.isAnyModelNode()) {
			continue;
		}
		if (!markKeyFramesChange(sceneGraph, entry->value)) {
			return false;
		}
	}
	return true;
}

bool MementoHandler::locked() {
	if (_locked > 0) {
		Log::debug("Don't add memento state - we are currently in locked mode");
		return true;
	}
	return false;
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

bool MementoHandler::markUndo(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
							  const voxel::RawVolume *volume, MementoType type, const voxel::Region &modifiedRegion) {
	const core::UUID &parentId = sceneGraph.uuid(node.parent());
	const core::UUID &referenceId = sceneGraph.uuid(node.reference());
	Log::debug("New memento state for node %s with name '%s'", node.uuid().str().c_str(), node.name().c_str());
	voxel::logRegion("MarkUndo", modifiedRegion);
	const MementoData &data = MementoData::fromVolume(volume, modifiedRegion);
	MementoState state(type, data, parentId, node.uuid(), referenceId, node.name(), node.type(), node.pivot(),
					   node.allKeyFrames(), node.palette(), node.normalPalette(), node.properties());
	if (node.hasIKConstraint()) {
		state.ikConstraint.setValue(*node.ikConstraint());
	}
	return addState(core::move(state));
}

bool MementoHandler::markUndo(const core::UUID &parentId, const core::UUID &nodeId, const core::UUID &referenceId,
							  const core::String &name, scenegraph::SceneGraphNodeType nodeType,
							  const voxel::RawVolume *volume, MementoType type, const voxel::Region &modifiedRegion,
							  const glm::vec3 &pivot, const scenegraph::SceneGraphKeyFramesMap &allKeyFrames,
							  const palette::Palette &palette, const palette::NormalPalette &normalPalette,
							  const scenegraph::SceneGraphNodeProperties &properties) {
	Log::debug("New memento state for node %s with name '%s'", nodeId.str().c_str(), name.c_str());
	voxel::logRegion("MarkUndo", modifiedRegion);
	const MementoData &data = MementoData::fromVolume(volume, modifiedRegion);
	MementoState state(type, data, parentId, nodeId, referenceId, name, nodeType, pivot, allKeyFrames, palette,
					   normalPalette, properties);
	return addState(core::move(state));
}

void MementoHandler::cutFromGroupStatePosition() {
	const int cutOff = core_max(0, (int)(stateSize() - _groupStatePosition - 1));
	Log::debug("Cut off %i states", cutOff);
	_groups.erase_back(cutOff);
}

bool MementoHandler::addState(MementoState &&state) {
	if (locked()) {
		for (auto *listener : _listeners) {
			listener->onMementoStateSkipped(state);
		}
		return false;
	}
	if (!_groups.empty()) {
		// if we mark something as new memento state, we can throw away
		// every other state that follows the new one (everything after
		// the current state position)
		const size_t n = _groups.size() - (_groupStatePosition + 1);
		_groups.erase_back(n);
	}
	core::ScopedLock lock(_mutex);
	if (_groupState > 0) {
		Log::debug("add group state: %i", _groupState);
		_groups.back().states.emplace_back(state);
		for (auto *listener : _listeners) {
			listener->onMementoStateAdded(_groups.back().states.back());
		}
		return true;
	}
	MementoStateGroup group;
	group.name = "single";
	group.states.emplace_back(state);
	cutFromGroupStatePosition();
	_groups.emplace_back(core::move(group));
	_groupStatePosition = stateSize() - 1;

	for (auto *listener : _listeners) {
		listener->onMementoStateAdded(_groups.back().states.back());
	}
	return true;
}

} // namespace memento
