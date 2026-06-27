/**
 * @file
 */

#include "Script.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/BrushGizmo.h"
#include "voxedit-util/modifier/brush/LUASelectionMode.h"

namespace voxedit {
namespace select {

voxel::Region Script::calcRegion(const BrushContext &ctx, const AABBBrushState &state) const {
	if (_activeLuaMode != nullptr && _activeLuaMode->hasGizmo()) {
		return voxel::Region::InvalidRegion;
	}
	return ctx.targetVolumeRegion;
}

bool Script::wantBrushGizmo(const BrushContext &ctx, const AABBBrushState &state) const {
	if (_activeLuaMode == nullptr || !state.boxMode) {
		return false;
	}
	return _activeLuaMode->wantBrushGizmo(ctx, state.aabbFirstPos, state.aabbFace);
}

void Script::brushGizmoState(const BrushContext &ctx, const AABBBrushState &state, BrushGizmoState &gizmoState) const {
	if (_activeLuaMode == nullptr) {
		gizmoState.operations = BrushGizmo_None;
		return;
	}
	_activeLuaMode->brushGizmoState(ctx, gizmoState, state.aabbFirstPos, state.aabbFace);
}

bool Script::needsAdditionalAction(const BrushContext &ctx) const {
	if (_activeLuaMode != nullptr && _activeLuaMode->hasGizmo()) {
		return true;
	}
	return Super::needsAdditionalAction(ctx);
}

void Script::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					  const voxel::Region &region, const AABBBrushState &state) {
	if (_activeLuaMode == nullptr) {
		return;
	}
	_activeLuaMode->execute(sceneGraph, wrapper, ctx, region, state.aabbFirstPos, state.aabbFace);
	if (_sceneManager == nullptr) {
		return;
	}
	const voxelgenerator::LuaDirtyRegions &dirtyRegions = _activeLuaMode->dirtyRegions();
	for (const auto &entry : dirtyRegions) {
		const int dirtyNodeId = entry->key;
		const voxel::Region &dirtyRegion = entry->value;
		if (dirtyNodeId == InvalidNodeId || !dirtyRegion.isValid()) {
			continue;
		}
		_sceneManager->modified(dirtyNodeId, dirtyRegion);
	}
}

} // namespace select
} // namespace voxedit
