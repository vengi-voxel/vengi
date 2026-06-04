/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include "voxel/Region.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace voxedit {

class SceneManager;

namespace select {

/**
 * @brief Selects the solid voxels along a line between the two drag endpoints.
 *
 * In PathMode::FollowSurface (default) the line is draped over the visible surface between the
 * endpoints (voxelutil::lineDrapeSurface): it marks the front-most surface at each step and fills
 * the riser faces of terrace steps, so it stays continuous AND visible across stepped/sloped
 * geometry. In PathMode::Straight it is a plain 3D chord that only selects the solid voxels it
 * passes through (voxelutil::lineMarkSolid). @c _width controls the thickness in both modes.
 *
 * While dragging, the line is previewed as a cheap world-space gizmo line (BrushGizmo_Line)
 * rather than a preview volume, so the preview never hits the preview-size cap and does not
 * vanish for long lines.
 * @ingroup Brushes
 */
class Line : public Strategy {
private:
	using Super = Strategy;
	SceneManager *_sceneManager;
	int _width = 1;
	/** FollowSurface: how far the surface may deviate from the straight 3D line and still be followed. */
	int _maxDeviation = 8;
	PathMode _pathMode = PathMode::FollowSurface;
	/** Live drag endpoints (node-local voxel coords) for the gizmo line preview. */
	glm::ivec3 _previewStart{0};
	glm::ivec3 _previewEnd{0};
	bool _previewActive = false;

public:
	explicit Line(SceneManager *sceneManager) : _sceneManager(sceneManager) {
	}

	voxel::Region calcRegion(const BrushContext &ctx, const AABBBrushState &state) const override {
		return voxel::Region::InvalidRegion;
	}
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	/** @brief A line is fully defined by the two drag endpoints, so no third placement click is needed. */
	bool needsAdditionalAction(const BrushContext &ctx) const override {
		return false;
	}

	void reset() override;
	void abort(BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;

	bool wantBrushGizmo(const BrushContext &ctx) const override;
	void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const override;

	/** Update the live gizmo-line endpoints during the drag. */
	void setPreview(const glm::ivec3 &start, const glm::ivec3 &end) {
		_previewStart = start;
		_previewEnd = end;
		_previewActive = true;
	}
	void clearPreview() {
		_previewActive = false;
	}

	int width() const {
		return _width;
	}

	void setWidth(int width) {
		_width = glm::clamp(width, 1, 32);
	}

	int maxDeviation() const {
		return _maxDeviation;
	}

	void setMaxDeviation(int deviation) {
		_maxDeviation = glm::clamp(deviation, 0, 64);
	}

	PathMode pathMode() const {
		return _pathMode;
	}

	void setPathMode(PathMode mode) {
		_pathMode = mode;
	}
};

} // namespace select
} // namespace voxedit
