/**
 * @file
 */

#pragma once

#include "voxel/ClipboardData.h"

namespace scenegraph {
class SceneGraphNode;
}

namespace voxedit {
namespace tool {

voxel::ClipboardData copy(const scenegraph::SceneGraphNode &node);
voxel::ClipboardData cut(scenegraph::SceneGraphNode &node, voxel::Region &modifiedRegion);
void paste(voxel::ClipboardData &out, const voxel::ClipboardData &in, const glm::ivec3 &referencePosition,
		   voxel::Region &modifiedRegion);

} // namespace tool
} // namespace voxedit
