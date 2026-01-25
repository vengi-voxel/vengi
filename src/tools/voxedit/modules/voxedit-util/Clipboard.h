/**
 * @file
 */

#pragma once

#include "modifier/SelectionManager.h"
#include "voxel/ClipboardData.h"

namespace scenegraph {
class SceneGraphNode;
}

namespace voxedit {
namespace tool {

voxel::ClipboardData copy(const scenegraph::SceneGraphNode &node, const SelectionManagerPtr &selectionMgr);
voxel::ClipboardData cut(scenegraph::SceneGraphNode &node, const SelectionManagerPtr &selectionMgr, voxel::Region &modifiedRegion);
void paste(voxel::ClipboardData &out, const voxel::ClipboardData &in, const glm::ivec3 &referencePosition,
		   voxel::Region &modifiedRegion);

} // namespace tool
} // namespace voxedit
