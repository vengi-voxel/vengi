/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/Optional.h"
#include "core/String.h"
#include "core/collection/RingBuffer.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <stddef.h>
#include <stdint.h>

namespace voxel {
class RawVolume;
}

namespace memento {

enum class MementoType {
	/**
	 * voxel volume modifications
	 */
	Modification,
	// scene graph actions
	SceneNodeMove,
	SceneNodeAdded,
	SceneNodeRemoved,
	SceneNodeRenamed,
	SceneNodePaletteChanged,
	SceneNodeKeyFrames,
	SceneNodeProperties,
	PaletteChanged,

	Max
};

/**
 * @brief Holds the data of a memento state
 *
 * The given buffer is owned by this class and represents a compressed volume
 */
class MementoData {
	friend struct MementoState;
	friend class MementoHandler;

private:
	/**
	 * @brief How big is the buffer with the compressed volume data
	 */
	size_t _compressedSize = 0;
	/**
	 * @brief The compressed volume data
	 */
	uint8_t *_buffer = nullptr;
	/**
	 * The region the given volume data is for
	 */
	voxel::Region _region{};

	MementoData(const uint8_t *buf, size_t bufSize, const voxel::Region &region);
	MementoData(uint8_t *buf, size_t bufSize, const voxel::Region &region);

public:
	MementoData() {
	}
	MementoData(MementoData &&o) noexcept;
	MementoData(const MementoData &o);
	~MementoData();

	inline size_t size() const {
		return _compressedSize;
	}

	MementoData &operator=(MementoData &&o) noexcept;
	MementoData &operator=(const MementoData &o) noexcept;

	inline const voxel::Region &region() const {
		return _region;
	}

	inline bool hasVolume() const {
		return _buffer != nullptr;
	}

	/**
	 * @brief Converts the given @c mementoData back into a voxels
	 * @note Inserts the voxels from the memento data into the given volume at the given region.
	 */
	static bool toVolume(voxel::RawVolume *volume, const MementoData &mementoData);
	/**
	 * @brief Converts the given volume into a @c MementoData structure (and perform the compression)
	 * @param[in] volume The volume to create the memento state for. This might be @c null.
	 * @param[in] region The region of the volume to create the memento data for - if this is not a valid region,
	 * the whole volume is going to added to the memento data.
	 */
	static MementoData fromVolume(const voxel::RawVolume *volume, const voxel::Region &region);
};

struct MementoState {
	MementoType type;
	// data is not always included in a state - as this is the volume and would consume a lot of memory
	MementoData data;

	// when re-adding nodes from a memento state, make sure to add them with the correct uuid
	core::String parentUUID;
	core::String nodeUUID;
	core::String referenceUUID;

	scenegraph::SceneGraphNodeType nodeType;
	scenegraph::SceneGraphKeyFramesMap keyFrames;
	scenegraph::SceneGraphNodeProperties properties;
	core::String name;
	glm::vec3 pivot;
	palette::Palette palette;

	MementoState() : type(MementoType::Max), nodeType(scenegraph::SceneGraphNodeType::Max) {
	}

	MementoState(const MementoState &other)
		: type(other.type), data(other.data), parentUUID(other.parentUUID), nodeUUID(other.nodeUUID),
		  referenceUUID(other.referenceUUID), nodeType(other.nodeType), keyFrames(other.keyFrames),
		  properties(other.properties), name(other.name), pivot(other.pivot), palette(other.palette) {
	}

	MementoState(MementoType type, const MementoState &other)
		: type(type), data(other.data), parentUUID(other.parentUUID), nodeUUID(other.nodeUUID), referenceUUID(other.referenceUUID),
		  nodeType(other.nodeType), keyFrames(other.keyFrames), properties(other.properties), name(other.name),
		  pivot(other.pivot), palette(other.palette) {
	}

	MementoState(MementoState &&other) noexcept {
		type = other.type;
		data = core::move(other.data);
		parentUUID = other.parentUUID;
		nodeUUID = other.nodeUUID;
		referenceUUID = other.referenceUUID;
		nodeType = other.nodeType;
		keyFrames = core::move(other.keyFrames);
		properties = core::move(other.properties);
		name = core::move(other.name);
		pivot = core::move(other.pivot);
		palette = core::move(other.palette);
	}

	MementoState &operator=(MementoState &&other) noexcept {
		if (&other == this) {
			return *this;
		}
		type = other.type;
		data = core::move(other.data);
		parentUUID = other.parentUUID;
		nodeUUID = other.nodeUUID;
		referenceUUID = other.referenceUUID;
		nodeType = other.nodeType;
		keyFrames = core::move(other.keyFrames);
		properties = core::move(other.properties);
		name = core::move(other.name);
		pivot = core::move(other.pivot);
		palette = core::move(other.palette);
		return *this;
	}

	MementoState &operator=(const MementoState &other) {
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
		return *this;
	}

	MementoState(MementoType _type, const MementoData &_data, const core::String &_parentId,
				 const core::String &_nodeId, const core::String &_referenceId, const core::String &_name,
				 scenegraph::SceneGraphNodeType _nodeType, const glm::vec3 &_pivot,
				 const scenegraph::SceneGraphKeyFramesMap &_keyFrames, const palette::Palette &_palette,
				 const scenegraph::SceneGraphNodeProperties &_properties)
		: type(_type), data(_data), parentUUID(_parentId), nodeUUID(_nodeId), referenceUUID(_referenceId),
		  nodeType(_nodeType), keyFrames(_keyFrames), properties(_properties), name(_name), pivot(_pivot),
		  palette(_palette) {
	}

	MementoState(MementoType _type, MementoData &&_data, core::String &&_parentId, core::String &&_nodeId,
				 core::String &&_referenceId, core::String &&_name, scenegraph::SceneGraphNodeType _nodeType,
				 glm::vec3 &&_pivot, scenegraph::SceneGraphKeyFramesMap &&_keyFrames, palette::Palette &&_palette,
				 scenegraph::SceneGraphNodeProperties &&_properties)
		: type(_type), data(_data), parentUUID(_parentId), nodeUUID(_nodeId), referenceUUID(_referenceId),
		  nodeType(_nodeType), keyFrames(_keyFrames), properties(_properties), name(_name), pivot(_pivot),
		  palette(_palette) {
	}

	inline bool valid() const {
		return type != MementoType::Max;
	}

	/**
	 * Some types (@c MementoType) don't have a volume attached.
	 */
	inline bool hasVolumeData() const {
		return data._buffer != nullptr;
	}

	inline const voxel::Region &dataRegion() const {
		return data._region;
	}
};

struct MementoStateGroup {
	core::String name;
	core::DynamicArray<MementoState> states;
};

using MementoStates = core::RingBuffer<MementoStateGroup, 64u>;
/**
 * @brief Class that manages the undo and redo steps for the scene
 *
 * @note For the volumes only the dirty regions are stored in a compressed form.
 */
class MementoHandler : public core::IComponent {
private:
	MementoStates _groups;
	int _groupState = 0;
	uint8_t _groupStatePosition = 0u;
	int _locked = 0;
	voxel::Region _maxUndoRegion = voxel::Region::InvalidRegion;

	void cutFromGroupStatePosition();
	void addState(MementoState &&state);
	/**
	 * @return @c true if it's allowed to create an undo state
	 */
	bool markUndoPreamble();

	// These undo methods search a valid state in the previous states and apply
	// the changes to the current state
	// The given MementoState is changed here
	void undoRename(MementoState &s);
	void undoMove(MementoState &s);
	void undoPaletteChange(MementoState &s);
	void undoNodeProperties(MementoState &s);
	void undoKeyFrames(MementoState &s);
	void undoModification(MementoState &s);

public:
	MementoHandler();
	~MementoHandler();

	void construct() override;
	bool init() override;
	void shutdown() override;

	/**
	 * @brief Allow to set the max region to record volume states for
	 */
	void setMaxUndoRegion(const voxel::Region &region);
	const voxel::Region &maxUndoRegion() const;
	/**
	 * @brief Checks if the given volume states are recorded
	 */
	bool recordVolumeStates(const voxel::RawVolume *volume) const;

	/**
	 * @brief Locks the handler for accepting new states or perform undo() or redo() steps
	 * @sa @c unlock()
	 */
	void lock();
	/**
	 * @brief Unlocks the handler for accepting new states or perform undo() or redo() steps again
	 * @sa @c lock()
	 */
	void unlock();

	void beginGroup(const core::String &name);
	void endGroup();

	void print() const;
	void printState(const MementoState &state) const;

	void clearStates();

	static const char *typeToString(MementoType type);

	/**
	 * @brief Add a new state entry to the memento handler that you can return to.
	 * @note This is adding the current active state to the handler - you can then undo to the previous state.
	 * That is the reason why you always have to add the initial (maybe empty) state, too
	 * @note Keep in mind, that there is a maximum of states that can get handled here.
	 * @param[in] volume The state of the volume
	 * @param[in] type The @c MementoType - has influence on undo() and redo() state position changes.
	 */
	bool markUndo(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node, const voxel::RawVolume *volume, MementoType type,
				  const voxel::Region &region);
	bool markUndo(const core::String &parentId, const core::String &nodeId, const core::String &referenceId,
				  const core::String &name, scenegraph::SceneGraphNodeType nodeType, const voxel::RawVolume *volume,
				  MementoType type, const voxel::Region &region, const glm::vec3 &pivot,
				  const scenegraph::SceneGraphKeyFramesMap &allKeyFrames, const palette::Palette &palette,
				  const scenegraph::SceneGraphNodeProperties &properties);
	bool removeLast();

	bool markNodePropertyChange(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markKeyFramesChange(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markNodeRemoved(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markNodeAdded(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markNodeTransform(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markModification(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node, const voxel::Region &modifiedRegion);
	bool markInitialNodeState(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markNodeRenamed(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markNodeMoved(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markPaletteChange(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markAddedAnimation(const scenegraph::SceneGraph& sceneGraph, const core::String &animation);
	bool markRemovedAnimation(const scenegraph::SceneGraph& sceneGraph, const core::String &animation);

	MementoStateGroup undo();
	MementoStateGroup redo();
	bool canUndo() const;
	bool canRedo() const;

	const MementoStateGroup &stateGroup() const;
	const MementoStates &states() const;

	size_t stateSize() const;
	uint8_t statePosition() const;
};

class ScopedMementoGroup {
private:
	MementoHandler &_handler;

public:
	ScopedMementoGroup(MementoHandler &handler, const core::String &name) : _handler(handler) {
		_handler.beginGroup(name);
	}
	~ScopedMementoGroup() {
		_handler.endGroup();
	}
};

/**
 * @brief Locks the memento handler to accept further state changes for undo/redo.
 * @note This is useful in situations where an undo or redo would result in actions that by
 * itself would generate new memento states, too.
 */
class ScopedMementoHandlerLock {
private:
	MementoHandler &_handler;

public:
	ScopedMementoHandlerLock(MementoHandler &handler) : _handler(handler) {
		_handler.lock();
	}
	~ScopedMementoHandlerLock() {
		_handler.unlock();
	}
};

class ScopedMementoHandlerUnlock {
private:
	MementoHandler &_handler;

public:
	ScopedMementoHandlerUnlock(MementoHandler &handler) : _handler(handler) {
		_handler.unlock();
	}
	~ScopedMementoHandlerUnlock() {
		_handler.lock();
	}
};

inline const MementoStateGroup &MementoHandler::stateGroup() const {
	return _groups[_groupStatePosition];
}

inline const MementoStates &MementoHandler::states() const {
	return _groups;
}

inline uint8_t MementoHandler::statePosition() const {
	return _groupStatePosition;
}

inline size_t MementoHandler::stateSize() const {
	return _groups.size();
}

inline bool MementoHandler::canUndo() const {
	if (_locked > 0) {
		return false;
	}
	if (stateSize() <= 1) {
		return false;
	}
	return _groupStatePosition > 0;
}

inline bool MementoHandler::canRedo() const {
	if (_locked > 0) {
		return false;
	}
	if (stateSize() <= 1) {
		return false;
	}
	return _groupStatePosition <= stateSize() - 2;
}

} // namespace memento
