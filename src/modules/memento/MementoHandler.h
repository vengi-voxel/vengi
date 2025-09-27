/**
 * @file
 */

#pragma once

#include "IMementoStateListener.h"
#include "core/IComponent.h"
#include "core/Optional.h"
#include "core/String.h"
#include "core/collection/RingBuffer.h"
#include "core/concurrent/Lock.h"
#include "palette/NormalPalette.h"
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

/**
 * @brief Enumeration of different types of memento states that can be tracked for undo/redo functionality
 *
 * Each type represents a specific kind of change that can be applied to the scene graph or voxel data.
 * The type determines how the undo/redo operations will restore the previous state.
 */
enum class MementoType {
	/**
	 * @brief Voxel volume modifications - changes to the actual voxel data within a volume
	 *
	 * This includes any changes to individual voxels, such as placing, removing, or modifying voxels.
	 * The memento state stores compressed volume data to restore the previous voxel configuration.
	 */
	Modification,

	/**
	 * @brief Scene graph node movement - changes to node position within the scene graph hierarchy
	 *
	 * Tracks when a node is moved to a different parent or position in the scene graph structure.
	 * Does not include transformations (rotation, scale, translation) of the node's content.
	 */
	SceneNodeMove,

	/**
	 * @brief Scene graph node addition - when a new node is added to the scene graph
	 *
	 * Records the creation of new nodes so they can be removed during undo operations.
	 * Stores all necessary information to recreate the node exactly as it was.
	 */
	SceneNodeAdded,

	/**
	 * @brief Scene graph node removal - when a node is deleted from the scene graph
	 *
	 * Records all node data so the node can be fully restored during undo operations.
	 * This includes the node's position, properties, volume data, and relationships.
	 */
	SceneNodeRemoved,

	/**
	 * @brief Scene graph node renaming - changes to a node's display name
	 *
	 * Tracks name changes for scene graph nodes so the previous name can be restored.
	 */
	SceneNodeRenamed,

	/**
	 * @brief Color palette changes for a scene graph node
	 *
	 * Records changes to the color palette associated with a node's voxel data.
	 * The palette defines the available colors that can be used for voxels in that node.
	 */
	SceneNodePaletteChanged,

	/**
	 * @brief Normal palette changes for a scene graph node
	 *
	 * Records changes to the normal palette used for lighting and surface calculations.
	 * Normal palettes define surface orientations for rendering purposes.
	 */
	SceneNodeNormalPaletteChanged,

	/**
	 * @brief Animation keyframe changes for a scene graph node
	 *
	 * Tracks modifications to animation keyframes that define how nodes transform over time.
	 * This includes position, rotation, and scale keyframes at different time points.
	 */
	SceneNodeKeyFrames,

	/**
	 * @brief Scene graph node property changes
	 *
	 * Records changes to various node properties such as visibility, transformation matrices,
	 * and other metadata associated with the node.
	 */
	SceneNodeProperties,

	/**
	 * @brief Scene graph animation changes
	 *
	 * Tracks changes to the overall animation system, such as adding or removing animations,
	 * or modifying animation properties that affect multiple nodes.
	 */
	SceneGraphAnimation,

	/**
	 * @brief Sentinel value indicating an invalid or uninitialized memento type
	 */
	Max
};

/**
 * @brief Holds compressed voxel volume data for a memento state
 *
 * The given buffer is owned by this class and represents a compressed volume
 *
 * The class distinguishes between two regions:
 * - dataRegion: The specific area within the volume that contains actual voxel data
 * - volumeRegion: The full bounds of the volume, which may be larger than the data region
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
	 * @brief Pointer to the compressed volume data buffer
	 *
	 * This buffer is owned by the MementoData instance and contains the compressed voxel data.
	 * The buffer is allocated when creating memento data and freed when the instance is destroyed.
	 * A nullptr indicates that no volume data is associated with this memento state.
	 */
	uint8_t *_buffer = nullptr;
	/**
	 * @brief The region within the volume that contains the actual voxel data
	 *
	 * This defines the bounds of the meaningful voxel data within the larger volume.
	 * For sparse volumes, this may be much smaller than the volume region, allowing
	 * for more efficient storage and processing.
	 */
	voxel::Region _dataRegion{};

	/**
	 * @brief The full bounds of the volume
	 *
	 * This defines the complete coordinate space of the volume, which may extend beyond
	 * the actual data region. This is important for maintaining proper spatial relationships
	 * when restoring volume data.
	 */
	voxel::Region _volumeRegion{};

	/**
	 * @brief The modified region is used for undoing.
	 */
	voxel::Region _modifiedRegion{};

	MementoData(const uint8_t *buf, size_t bufSize, const voxel::Region &dataRegion, const voxel::Region &volumeRegion);
	MementoData(uint8_t *buf, size_t bufSize, const voxel::Region &dataRegion, const voxel::Region &volumeRegion);

public:
	MementoData() {
	}
	MementoData(MementoData &&o) noexcept;
	MementoData(const MementoData &o);
	~MementoData();

	/**
	 * @brief Get the size of the compressed data buffer
	 * @return Size in bytes of the compressed data, 0 if no data is present
	 */
	inline size_t size() const {
		return _compressedSize;
	}

	MementoData &operator=(MementoData &&o) noexcept;
	MementoData &operator=(const MementoData &o) noexcept;

	/**
	 * @brief Get the region containing actual voxel data
	 * @return The data region bounds
	 */
	inline const voxel::Region &dataRegion() const {
		return _dataRegion;
	}

	/**
	 * @brief Get the full volume bounds
	 * @return The complete volume region bounds
	 */
	inline const voxel::Region &volumeRegion() const {
		return _volumeRegion;
	}

	/**
	 * @brief Check if this memento data contains volume information
	 * @return true if volume data is present, false if this is a metadata-only memento
	 */
	inline bool hasVolume() const {
		return _buffer != nullptr;
	}

	/**
	 * @brief Get read-only access to the compressed data buffer
	 * @return Pointer to the compressed data buffer, or nullptr if no data is present
	 */
	const uint8_t *buffer() const {
		return _buffer;
	}

	void setModifiedRegion(const voxel::Region &region) {
		_modifiedRegion = region;
	}
	const voxel::Region &modifiedRegion() const {
		return _modifiedRegion;
	}

	/**
	 * @brief Decompresses and restores voxel data from memento state into a volume
	 *
	 * This method takes the compressed voxel data stored in the memento and decompresses it
	 * back into the target volume. The voxels are inserted at the positions specified by
	 * the memento's data region.
	 *
	 * @param[in,out] volume The target volume to restore voxel data into
	 * @param[in] mementoData The memento data containing compressed voxel information
	 * @return true if the restoration was successful, false on decompression or other errors
	 */
	static bool toVolume(voxel::RawVolume *volume, const MementoData &mementoData, const voxel::Region &region);
	/**
	 * @brief Compresses volume data into a MementoData structure for storage
	 *
	 * This method takes a voxel volume and compresses the specified region into a compact
	 * representation suitable for undo/redo storage. The compression significantly reduces
	 * memory usage compared to storing raw voxel data.
	 *
	 * @param[in] volume The source volume to compress. Can be nullptr for empty memento data.
	 * @param[in] region The specific region within the volume to compress. If invalid,
	 *                   the entire volume bounds will be used.
	 * @return MementoData instance containing the compressed volume data and region information
	 */
	static MementoData fromVolume(const voxel::RawVolume *volume, const voxel::Region &region);
};

/**
 * @brief Complete snapshot of a scene graph node's state for undo/redo functionality
 *
 * This structure captures all relevant information about a scene graph node at a specific point in time.
 * Not all fields are meaningful for every MementoType - the relevant fields depend on the type of
 * change being tracked. For example, a SceneNodeRenamed memento only needs the name field, while
 * a Modification memento requires the compressed volume data.
 */
struct MementoState {
	/**
	 * @brief The type of change this memento represents
	 *
	 * This determines which fields in the structure are meaningful and how the undo/redo
	 * operation should be performed.
	 */
	MementoType type;
	// data is not always included in a state - as this is the volume and would consume a lot of memory
	MementoData data;

	/**
	 * @brief UUID of the parent node in the scene graph hierarchy
	 *
	 * Used to maintain proper parent-child relationships when restoring nodes.
	 * Essential for SceneNodeAdded and SceneNodeRemoved operations to ensure
	 * nodes are restored to the correct position in the hierarchy.
	 */
	core::UUID parentUUID;

	/**
	 * @brief Unique identifier for this scene graph node
	 *
	 * Every scene graph node has a persistent UUID that remains constant across
	 * operations. This is crucial for tracking the same logical node across
	 * different memento states.
	 */
	core::UUID nodeUUID;

	/**
	 * @brief UUID of a referenced node (for reference-type nodes)
	 *
	 * Some nodes in the scene graph can reference other nodes rather than containing
	 * their own data. This field stores the UUID of the referenced node when applicable.
	 */
	core::UUID referenceUUID;

	/**
	 * @brief The type of scene graph node (Model, Group, Camera, etc.)
	 *
	 * Different node types have different properties and behaviors. This information
	 * is essential for correctly recreating nodes during undo operations.
	 */
	scenegraph::SceneGraphNodeType nodeType;

	/**
	 * @brief Animation keyframes associated with this node
	 *
	 * Stores transformation keyframes (position, rotation, scale) at different time points.
	 * This enables undoing changes to node animations and maintaining temporal consistency.
	 */
	scenegraph::SceneGraphKeyFramesMap keyFrames;

	/**
	 * @brief Various properties and metadata for the scene graph node
	 */
	scenegraph::SceneGraphNodeProperties properties;

	/**
	 * @brief Display name of the scene graph node
	 */
	core::String name;

	/**
	 * @brief Pivot point for transformations and rotations
	 */
	glm::vec3 pivot;
	palette::Palette palette;
	palette::NormalPalette normalPalette;

	/**
	 * @brief List of strings for various purposes (e.g., animation names)
	 *
	 * A flexible field used to store arrays of strings for different purposes.
	 * For example, it may contain the names of animations in the scene graph.
	 * The Optional wrapper indicates this field is not always populated.
	 */
	core::Optional<core::DynamicArray<core::String>> stringList;

	MementoState();
	MementoState(const MementoState &other);
	MementoState(MementoType _type, const MementoState &other);
	MementoState(MementoState &&other) noexcept;
	MementoState &operator=(MementoState &&other) noexcept;
	MementoState &operator=(const MementoState &other);
	MementoState(MementoType _type, const MementoData &_data, const core::UUID &_parentId,
				 const core::UUID &_nodeId, const core::UUID &_referenceId, const core::String &_name,
				 scenegraph::SceneGraphNodeType _nodeType, const glm::vec3 &_pivot,
				 const scenegraph::SceneGraphKeyFramesMap &_keyFrames, const palette::Palette &_palette,
				 const palette::NormalPalette &_normalPalette, const scenegraph::SceneGraphNodeProperties &_properties);
	MementoState(MementoType _type, MementoData &&_data, core::UUID &&_parentId, core::UUID &&_nodeId,
				 core::UUID &&_referenceId, core::String &&_name, scenegraph::SceneGraphNodeType _nodeType,
				 glm::vec3 &&_pivot, scenegraph::SceneGraphKeyFramesMap &&_keyFrames, palette::Palette &&_palette,
				 palette::NormalPalette &&_normalPalette, scenegraph::SceneGraphNodeProperties &&_properties);
	MementoState(MementoType _type, const core::DynamicArray<core::String> &stringList);

	/**
	 * @brief Check if this memento state is valid and can be used for undo/redo
	 * @return true if the state has a valid type, false if uninitialized or invalid
	 */
	inline bool valid() const {
		return type != MementoType::Max;
	}

	/**
	 * @brief Check if this memento state contains compressed volume data
	 *
	 * Not all memento types require volume data. For example, SceneNodeRenamed only needs
	 * the name change, while Modification requires the actual voxel data to restore.
	 *
	 * @return true if compressed volume data is present, false for metadata-only changes
	 */
	inline bool hasVolumeData() const {
		return data._buffer != nullptr;
	}

	/**
	 * @brief Get the region containing actual voxel data
	 * @return The bounds of meaningful voxel data within the volume
	 */
	inline const voxel::Region &dataRegion() const {
		return data._dataRegion;
	}

	/**
	 * @brief Get the full volume bounds
	 * @return The complete coordinate space of the volume
	 */
	inline const voxel::Region &volumeRegion() const {
		return data._volumeRegion;
	}
};

/**
 * @brief A collection of related memento states that should be treated as a single undo/redo operation
 *
 * When multiple changes occur together (such as moving a node and updating its properties),
 * they are grouped together so that undoing/redoing affects all related changes atomically.
 * This prevents partial undo states that could leave the scene in an inconsistent condition.
 *
 * @see ScopedMementoGroup for convenient group management
 */
struct MementoStateGroup {
	core::String name;

	/**
	 * @brief Array of individual memento states that comprise this group
	 *
	 * When undoing/redoing, all states in this array are processed together in the
	 * appropriate order to restore the complete previous state.
	 */
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
	/**
	 * we lock the memento state handler for new states while we are performing an undo or redo step
	 */
	int _locked = 0;
	core_trace_mutex(core::Lock, _mutex, "MementoHandler");

	// Network notification listeners
	core::DynamicArray<IMementoStateListener *> _listeners;

	void cutFromGroupStatePosition();
	bool addState(MementoState &&state);
	/**
	 * @return @c true if it's not allowed to create a new undo state
	 */
	bool locked();

	// we should not all this method directly - must be part of a group
	bool markAllAnimations(const core::DynamicArray<core::String> &animations);

	// These undo methods search a valid state in the previous states and apply
	// the changes to the current state
	// The given MementoState is changed here
	void undoRename(MementoState &s);
	void undoMove(MementoState &s);
	void undoPaletteChange(MementoState &s);
	void undoNormalPaletteChange(MementoState &s);
	void undoNodeProperties(MementoState &s);
	void undoKeyFrames(MementoState &s);
	void undoAnimations(MementoState &s);
	void undoModification(MementoState &s);

protected:
	/**
	 * @brief Add a new state entry to the memento handler that you can return to.
	 * @note This is adding the current active state to the handler - you can then undo to the previous state.
	 * That is the reason why you always have to add the initial (maybe empty) state, too
	 * @note Keep in mind, that there is a maximum of states that can get handled here.
	 * @param[in] volume The state of the volume
	 * @param[in] type The @c MementoType - has influence on undo() and redo() state position changes.
	 */
	bool markUndo(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
				  const voxel::RawVolume *volume, MementoType type, const voxel::Region &modifiedRegion);
	bool markUndo(const core::UUID &parentId, const core::UUID &nodeId, const core::UUID &referenceId,
				  const core::String &name, scenegraph::SceneGraphNodeType nodeType, const voxel::RawVolume *volume,
				  MementoType type, const voxel::Region &modifiedRegion, const glm::vec3 &pivot,
				  const scenegraph::SceneGraphKeyFramesMap &allKeyFrames, const palette::Palette &palette,
				  const palette::NormalPalette &normalPalette, const scenegraph::SceneGraphNodeProperties &properties);

public:
	MementoHandler();
	~MementoHandler();

	void construct() override;
	bool init() override;
	void shutdown() override;

	/**
	 * @brief Add a listener for memento state changes
	 * @param listener The listener to add (must remain valid until removed)
	 */
	void registerListener(IMementoStateListener *listener);

	/**
	 * @brief Remove a listener for memento state changes
	 * @param listener The listener to remove
	 */
	void unregisterListener(IMementoStateListener *listener);

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

	bool removeLast();

	bool markNodePropertyChange(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markKeyFramesChange(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markNodeRemove(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markNodeAdded(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markNodeTransform(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markModification(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
						  const voxel::Region &modifiedRegion);
	bool markInitialNodeState(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markInitialSceneState(const scenegraph::SceneGraph &sceneGraph);
	bool markNodeRenamed(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markNodeMoved(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markPaletteChange(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markNormalPaletteChange(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);
	bool markAnimationAdded(const scenegraph::SceneGraph &sceneGraph, const core::String &animation);
	bool markAnimationRemoved(const scenegraph::SceneGraph &sceneGraph, const core::String &animation);

	void extractVolumeRegion(voxel::RawVolume *targetVolume, const MementoState &state) const;

	/**
	 * @brief This returns the state we are moving to
	 */
	MementoStateGroup undo();
	/**
	 * @brief This returns the state we are moving to
	 */
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
