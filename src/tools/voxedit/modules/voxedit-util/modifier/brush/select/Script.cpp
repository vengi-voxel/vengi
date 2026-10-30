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
#include "voxedit-util/modifier/brush/ScriptBrushTarget.h"

namespace voxedit {
namespace select {

voxel::Region Script::calcRegion(const BrushContext &ctx, const AABBBrushState &state) const {
	if (_activeLuaMode != nullptr && _activeLuaMode->hasGizmo()) {
		return voxel::Region::InvalidRegion;
	}
	return ctx.targetVolumeRegion;
}

bool Script::hasHandle(const BrushContext &ctx, const AABBBrushState &state) const {
	if (_activeLuaMode == nullptr || !state.boxMode) {
		return false;
	}
	return _activeLuaMode->hasHandle(ctx, state.aabbFirstPos, state.aabbFace);
}

void Script::describeHandle(const BrushContext &ctx, const AABBBrushState &state, BrushGizmoState &gizmoState) const {
	if (_activeLuaMode == nullptr) {
		gizmoState.operations = BrushGizmo_None;
		return;
	}
	_activeLuaMode->describeHandle(ctx, gizmoState, state.aabbFirstPos, state.aabbFace);
}

bool Script::needsAdditionalAction(const BrushContext &ctx) const {
	if (_activeLuaMode != nullptr && _activeLuaMode->hasGizmo()) {
		return true;
	}
	return Super::needsAdditionalAction(ctx);
}

void Script::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					  const voxel::Region &region, const AABBBrushState &state) {
	evaluate(sceneGraph, wrapper, ctx, region, state);
}

bool Script::evaluate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					  const voxel::Region &region, const AABBBrushState &state) {
	if (_activeLuaMode == nullptr) {
		return false;
	}
	bool success = false;
	const bool valid = evaluateScriptTarget(wrapper, [&](scenegraph::SceneGraph &scene, ModifierVolumeWrapper &target) {
		LUASelectionMode evaluation(*_activeLuaMode);
		success = evaluation.execute(scene, target, ctx, region, state.aabbFirstPos, state.aabbFace);
	});
	return valid && success;
}

} // namespace select
} // namespace voxedit
