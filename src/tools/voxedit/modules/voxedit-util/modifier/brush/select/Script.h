/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include "voxedit-util/modifier/brush/BrushGizmo.h"

namespace voxedit {

class SceneManager;
class LUASelectionMode;

namespace select {

class Script : public Strategy {
private:
	using Super = Strategy;
	SceneManager *_sceneManager;
	LUASelectionMode *_activeLuaMode = nullptr;

public:
	explicit Script(SceneManager *sceneManager) : _sceneManager(sceneManager) {
	}

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	voxel::Region calcRegion(const BrushContext &ctx, const AABBBrushState &state) const override;

	bool wantBrushGizmo(const BrushContext &ctx, const AABBBrushState &state) const override;
	void brushGizmoState(const BrushContext &ctx, const AABBBrushState &state, BrushGizmoState &gizmoState) const override;
	bool needsAdditionalAction(const BrushContext &ctx) const override;

	LUASelectionMode *activeLuaMode() const {
		return _activeLuaMode;
	}
	void setActiveLuaMode(LUASelectionMode *mode) {
		_activeLuaMode = mode;
	}
};

} // namespace select
} // namespace voxedit
