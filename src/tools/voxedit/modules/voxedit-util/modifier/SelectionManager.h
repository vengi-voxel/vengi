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
public:
	void reset();
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
	 * @note The caller has to take care about the memory of the returned volume
	 */
	[[nodiscard]] voxel::RawVolume *cut(scenegraph::SceneGraphNode &node) const;
	/**
	 * @brief Copy selected voxels from the given node and return a new volume containing them
	 * @note The caller has to take care about the memory of the returned volume
	 */
	[[nodiscard]] voxel::RawVolume *copy(const scenegraph::SceneGraphNode &node) const;
};

using SelectionManagerPtr = core::SharedPtr<SelectionManager>;

} // namespace voxedit
