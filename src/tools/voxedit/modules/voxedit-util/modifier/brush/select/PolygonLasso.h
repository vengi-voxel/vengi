/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include "core/GLM.h"
#include "core/collection/DynamicArray.h"
#include "voxel/Face.h"
#include <glm/vec3.hpp>

namespace voxel {
class RawVolume;
} // namespace voxel

namespace voxedit {

class SceneManager;

namespace select {

/**
 * @brief Click-vertex polygonal lasso with surface flood-fill on close.
 *
 * Distinct from the screen-space drag-fill Lasso strategy. The user clicks individual
 * voxels to drop polygon vertices; the in-progress polygon is shown as a lightweight
 * world-space overlay polyline (BrushGizmo_WorldPolyline) rather than by writing marks
 * onto the volume - so accumulation stays cheap on large models. On close (click near
 * the first vertex) surface voxels are flood-filled from edge seeds inside the polygon,
 * avoiding disjoint structures that share the (u, v) silhouette.
 */
class PolygonLasso : public Strategy {
public:
	/** Max U/V distance from the first vertex that auto-closes the polygon on click */
	static constexpr int CloseThresholdVoxels = 1;
	/** Initial capacity reserved for the polygon vertex list */
	static constexpr int PathInitialReserve = 32;
	/** Sentinel value for the last-cursor-pos check to force a refresh on first update() */
	static constexpr int InvalidCursorCoord = -100000;

private:
	using Super = Strategy;
	SceneManager *_sceneManager;
	/** Polygon vertices accumulated across clicks */
	core::DynamicArray<glm::ivec3> _path;
	/** World-space points for the viewport overlay polyline: all vertices plus the live cursor */
	core::DynamicArray<glm::vec3> _overlayPoints;
	/** True when the cursor is near the first vertex so a click would close the polygon */
	bool _overlayClosable = false;
	bool _accumulating = false;
	int _uAxis = 0;
	int _vAxis = 1;
	int _faceAxisIdx = 2;
	/** Max depth the surface may deviate from the fitted lasso plane and still be filled */
	int _depthTolerance = 4;
	voxel::FaceNames _face = voxel::FaceNames::Max;
	glm::ivec3 _lastCursorPos{InvalidCursorCoord};

	static glm::ivec3 snapToGrid(const glm::ivec3 &pos, int resolution);
	static void ellipseAxes(voxel::FaceNames face, int &uAxis, int &vAxis);
	/** Rebuild the world-space overlay polyline from the current path plus the live cursor */
	void buildOverlayPoints(const BrushContext &ctx);

public:
	explicit PolygonLasso(SceneManager *sceneManager) : _sceneManager(sceneManager) {
	}

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	voxel::Region calcRegion(const BrushContext &ctx, const AABBBrushState &state) const override;
	bool beginBrush(const BrushContext &ctx, const AABBBrushState &state) override;
	void abort(BrushContext &ctx) override;
	void reset() override;
	void update(const BrushContext &ctx, double nowSeconds) override;
	bool active() const override {
		return _accumulating;
	}
	bool needsAdditionalAction(const BrushContext &ctx) const override {
		return false;
	}

	bool wantBrushGizmo(const BrushContext &ctx) const override;
	void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const override;

	bool accumulating() const {
		return _accumulating;
	}
	const core::DynamicArray<glm::ivec3> &path() const {
		return _path;
	}
	int uAxis() const {
		return _uAxis;
	}
	int vAxis() const {
		return _vAxis;
	}
	int faceAxisIdx() const {
		return _faceAxisIdx;
	}
	voxel::FaceNames face() const {
		return _face;
	}
	int depthTolerance() const {
		return _depthTolerance;
	}
	void setDepthTolerance(int tolerance) {
		_depthTolerance = glm::clamp(tolerance, 0, 32);
	}

	/** Discard the in-progress polygon */
	void invalidate();
	/** Remove the last placed polygon vertex */
	void popLastPathEntry();
};

} // namespace select
} // namespace voxedit
