/**
 * @file
 */

#pragma once

#include "core/DirtyState.h"
#include "core/SharedPtr.h"
#include "voxel/Region.h"

namespace scenegraph {
class SceneGraphNode;
}

namespace voxel {
class RawVolume;
}

namespace voxedit {

// TODO: SELECTION: see https://github.com/vengi-voxel/vengi/issues/580
class SelectionManager : public core::DirtyState {
private:
	// when moving selected voxels, don't do it in a region larger than this
	voxel::Region _maxRegion = voxel::Region::InvalidRegion;

public:
	void reset();
	void setMaxRegionSize(const voxel::Region &maxRegion);
	voxel::Region calculateRegion(const scenegraph::SceneGraphNode &node) const;

	void invert(scenegraph::SceneGraphNode &node);
	bool select(scenegraph::SceneGraphNode &node, const glm::ivec3 &mins, const glm::ivec3 &maxs);
	bool select(scenegraph::SceneGraphNode &node, const glm::ivec3 &pos);
	void selectAll(scenegraph::SceneGraphNode &node);
	/**
	 * @brief Unselect all selected voxels in the given node
	 */
	void unselect(scenegraph::SceneGraphNode &node);
	bool unselect(scenegraph::SceneGraphNode &node, const glm::ivec3 &pos);
	bool unselect(scenegraph::SceneGraphNode &node, const glm::ivec3 &mins, const glm::ivec3 &maxs);
	bool isSelected(const scenegraph::SceneGraphNode &node, const glm::ivec3 &pos) const;
	/**
	 * @brief Cut selected voxels from the given node and return a new volume containing them
	 */
	voxel::RawVolume *cut(scenegraph::SceneGraphNode &node);
	/**
	 * @brief Copy selected voxels from the given node and return a new volume containing them
	 */
	voxel::RawVolume *copy(const scenegraph::SceneGraphNode &node);
};

using SelectionManagerPtr = core::SharedPtr<SelectionManager>;

} // namespace voxedit
