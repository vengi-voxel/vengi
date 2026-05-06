/**
 * @file
 */

#include "Surface.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {
namespace select {

void Surface::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					   const voxel::Region &region, const AABBBrushState &state) {
	auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &voxel) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};
	voxelutil::visitSurfaceVolumeParallel(wrapper, func);
}

} // namespace select
} // namespace voxedit
