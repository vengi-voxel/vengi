/**
 * @file
 */

#include "Script.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/LUASelectionMode.h"

namespace voxedit {
namespace select {

voxel::Region Script::calcRegion(const BrushContext &ctx, const AABBBrushState &state) const {
	return ctx.targetVolumeRegion;
}

void Script::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					  const voxel::Region &region, const AABBBrushState &state) {
	if (_activeLuaMode == nullptr) {
		return;
	}
	_activeLuaMode->execute(sceneGraph, wrapper, ctx, region, state.aabbFirstPos, state.aabbFace);
	if (_sceneManager) {
		const voxelgenerator::LuaDirtyRegions &dirtyRegions = _activeLuaMode->dirtyRegions();
		for (const auto &entry : dirtyRegions) {
			const int dirtyNodeId = entry->key;
			const voxel::Region &dirtyRegion = entry->value;
			if (dirtyRegion.isValid()) {
				_sceneManager->modified(dirtyNodeId, dirtyRegion);
			}
		}
	}
}

} // namespace select
} // namespace voxedit
