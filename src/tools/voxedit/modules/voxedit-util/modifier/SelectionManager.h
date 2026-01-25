/**
 * @file
 */

#pragma once

#include "Selection.h"
#include "core/DirtyState.h"
#include "core/SharedPtr.h"

namespace voxel {
class RawVolume;
}

namespace voxedit {

// TODO: SELECTION: the plan here is to move the selected voxels into the sparse volume to allow copy/cut/move operations
//                  see https://github.com/vengi-voxel/vengi/issues/580
//                  UPDATE: Turns out this isn't the best solution as it would require to also ray trace against a sparse
//                  volume in many places. Maybe it's better to use a BitVolume to only mark selected voxels. In that case
//                  we can make all volume parameters here const and only modify the selection state. We should need a
//                  way to e.g. move the selected voxels - and also update the BitVolume accordingly, as the selection would
//                  move along with the voxels. All in all the SelectionManager needs a completly new design then.
// TODO: SELECTION: When using a Sparse or a BitVolume, we should unselect on node switch, as the selection is per node
// TODO: SELECTION: instead of resetting a selection it might make sense to keep the last x selections for different
//                  scene graph nodes in a RingBuffer - so that switching between nodes keeps the selection state of previous
//                  nodes.
//                  E.g. like this:
//
//                  struct SelectionState {
//                    voxel::BitVolume selectedVoxels;
//                    core::String nodeUUID;
//                  };
//                  core::RingBuffer<SelectionState, 5> _selectionStates;
//                  void activateNode(const core::String &nodeUUID);
//
//                  nodeActivate should get called in the SceneManager class when a node is activated. Either looking up a previous
//                  selection state or creating a new one for the node.
class SelectionManager : public core::DirtyState {
private:
	Selections _selections;
	// when moving selected voxels, don't do it in a region larger than this
	voxel::Region _maxRegion = voxel::Region::InvalidRegion;
	voxel::Region _cachedRegion = voxel::Region::InvalidRegion;

public:
	// TODO: SELECTION: reduce access to this as much as possible
	const Selections &selections() const;

	void setMaxRegionSize(const voxel::Region &maxRegion);
	const voxel::Region& region();
	bool hasSelection() const;

	void invert(voxel::RawVolume &volume);
	bool select(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs);
	bool select(voxel::RawVolume &volume, const glm::ivec3 &pos);
	void selectAll(voxel::RawVolume &volume);
	/**
	 * @brief Unselect all selected voxels in the given volume
	 */
	void unselect(voxel::RawVolume &volume);
	bool unselect(voxel::RawVolume &volume, const glm::ivec3 &pos);
	bool unselect(voxel::RawVolume &volume, const glm::ivec3 &mins, const glm::ivec3 &maxs);
	bool isSelected(const glm::ivec3 &pos) const;
	/**
	 * @brief Cut selected voxels from the given volume and return a new volume containing them
	 */
	voxel::RawVolume *cut(voxel::RawVolume &volume);
	/**
	 * @brief Copy selected voxels from the given volume and return a new volume containing them
	 */
	voxel::RawVolume *copy(const voxel::RawVolume &volume);

	/**
	 * @brief Unselect everything
	 */
	void reset();
};

inline bool SelectionManager::hasSelection() const {
	return !_selections.empty();
}

using SelectionManagerPtr = core::SharedPtr<SelectionManager>;

} // namespace voxedit
