/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include "core/collection/DynamicArray.h"
#include "voxel/Region.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace voxedit {

class SceneManager;

namespace select {

/**
 * @brief Selects a 2D rectangle outline drawn on the clicked surface.
 *
 * The two drag corners define a rectangle in the clicked face plane that can be rotated by
 * @c _angle degrees. Its four edges are draped over the visible surface (voxelutil::lineDrapeSurface)
 * with thickness @c _edgeWidth, filling terrace riser faces so each edge stays continuous AND
 * visible. While dragging it is previewed as a cheap world-space outline (BrushGizmo_WorldPolyline)
 * so the preview never hits the preview-size cap and shows the rotation.
 * @ingroup Brushes
 */
class Rectangle : public Strategy {
private:
	using Super = Strategy;
	SceneManager *_sceneManager;
	/** Thickness in voxels of the rectangle outline. */
	int _edgeWidth = 1;
	/** In-plane rotation of the rectangle around the clicked face normal, in degrees. */
	int _angle = 0;
	/** How far the surface may deviate from the straight edge and still be followed. */
	int _maxDeviation = 64;
	/** Live world-space corners for the gizmo outline preview (rebuilt each frame while dragging). */
	core::DynamicArray<glm::vec3> _overlayPoints;
	bool _previewActive = false;

	/** Compute the four rotated rectangle corners (node-local) for the given drag box and face. */
	void computeCorners(const voxel::Region &region, voxel::FaceNames face, glm::ivec3 corners[4]) const;

public:
	explicit Rectangle(SceneManager *sceneManager) : _sceneManager(sceneManager) {
	}

	voxel::Region calcRegion(const BrushContext &ctx, const AABBBrushState &state) const override {
		return voxel::Region::InvalidRegion;
	}
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	/** @brief A 2D rectangle is defined by the two drag corners, so no third placement click is needed. */
	bool needsAdditionalAction(const BrushContext &ctx) const override {
		return false;
	}

	void reset() override;
	void abort(BrushContext &ctx) override;
	void endBrush(BrushContext &ctx) override;
	bool wantBrushGizmo(const BrushContext &ctx) const override;
	void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const override;
	/** Update the live gizmo outline from the drag box and clicked face. */
	void setPreview(const voxel::Region &region, voxel::FaceNames face);
	void clearPreview() {
		_previewActive = false;
	}

	int edgeWidth() const {
		return _edgeWidth;
	}

	void setEdgeWidth(int width) {
		_edgeWidth = glm::clamp(width, 1, 32);
	}

	int angle() const {
		return _angle;
	}

	void setAngle(int angle) {
		_angle = glm::clamp(angle, -180, 180);
	}

	int maxDeviation() const {
		return _maxDeviation;
	}

	void setMaxDeviation(int deviation) {
		_maxDeviation = glm::clamp(deviation, 0, 64);
	}
};

} // namespace select
} // namespace voxedit
