/**
 * @file
 */

#include "ISceneRenderer.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

const voxel::RawVolume *ISceneRenderer::volumeForNode(const scenegraph::SceneGraphNode &node) {
	return node.volume();
}

} // namespace voxedit
