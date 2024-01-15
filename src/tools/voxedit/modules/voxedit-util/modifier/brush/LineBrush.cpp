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
	const ModifierType modifierType = wrapper.modifierType();
	const bool isErase = (modifierType & ModifierType::Erase) == ModifierType::Erase;
	const bool isPaint = (modifierType & ModifierType::Paint) == ModifierType::Paint;
	if (isErase) {
		voxel = voxel::createVoxel(voxel::VoxelType::Air, 0);
	}
	voxelutil::raycastWithEndpoints(&wrapper, start, end, [=](auto &sampler) {
		const bool air = voxel::isAir(sampler.voxel().getMaterial());
		if ((!isErase && !isPaint) && !air) {
			return true;
		}
		sampler.setVoxel(voxel);
		return true;
	});
	return true;
}

} // namespace voxedit
