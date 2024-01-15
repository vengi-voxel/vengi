/**
 * @file
 */

#include "LineBrush.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelutil/Raycast.h"

namespace voxedit {

bool LineBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						const BrushContext &context) {
	const glm::ivec3 &start = context.referencePos;
	const glm::ivec3 &end = context.cursorPosition;
	voxel::Voxel voxel = context.cursorVoxel;
	voxelutil::raycastWithEndpoints(&wrapper, start, end, [&](auto &sampler) {
		const glm::ivec3 &pos = sampler.position();
		wrapper.setVoxel(pos.x, pos.y, pos.z, voxel);
		return true;
	});
	return true;
}

} // namespace voxedit
