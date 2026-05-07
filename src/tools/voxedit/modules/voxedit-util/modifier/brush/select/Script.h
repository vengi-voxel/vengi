/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"

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

	LUASelectionMode *activeLuaMode() const {
		return _activeLuaMode;
	}
	void setActiveLuaMode(LUASelectionMode *mode) {
		_activeLuaMode = mode;
	}
};

} // namespace select
} // namespace voxedit
