/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include "core/collection/DynamicArray.h"
#include <glm/vec2.hpp>

namespace voxedit {

class SceneManager;

namespace select {

class Lasso : public Strategy {
private:
	using Super = Strategy;
	SceneManager *_sceneManager;
	core::DynamicArray<glm::vec2> _screenPoints;
	bool _screenDragging = false;

public:
	explicit Lasso(SceneManager *sceneManager) : _sceneManager(sceneManager) {
	}

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	bool beginBrush(const BrushContext &ctx, const AABBBrushState &state) override;
	void endBrush(BrushContext &ctx) override;
	void abort(BrushContext &ctx) override;
	void reset() override;
	void update(const BrushContext &ctx, double nowSeconds) override;
	bool active() const override;

	bool wantBrushGizmo(const BrushContext &ctx, const AABBBrushState &state) const override;
	void brushGizmoState(const BrushContext &ctx, const AABBBrushState &state, BrushGizmoState &gizmoState) const override;

	bool screenDragging() const {
		return _screenDragging;
	}
	const core::DynamicArray<glm::vec2> &screenPoints() const {
		return _screenPoints;
	}
};

} // namespace select
} // namespace voxedit
