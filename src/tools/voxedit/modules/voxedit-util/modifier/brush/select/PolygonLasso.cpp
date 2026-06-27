/**
 * @file
 */

#include "PolygonLasso.h"
#include "Circle.h"
#include "math/Axis.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/BrushGizmo.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeSelect.h"
#include <glm/common.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace voxedit {
namespace select {

glm::ivec3 PolygonLasso::snapToGrid(const glm::ivec3 &pos, int resolution) {
	if (resolution <= 1) {
		return pos;
	}
	glm::ivec3 out = pos;
	if (out.x % resolution != 0) {
		out.x = (out.x / resolution) * resolution;
	}
	if (out.y % resolution != 0) {
		out.y = (out.y / resolution) * resolution;
	}
	if (out.z % resolution != 0) {
		out.z = (out.z / resolution) * resolution;
	}
	return out;
}

void PolygonLasso::ellipseAxes(voxel::FaceNames face, int &uAxis, int &vAxis) {
	Circle::ellipseAxes(face, uAxis, vAxis);
}

void PolygonLasso::reset() {
	_path.clear();
	_overlayPoints.clear();
	_overlayClosable = false;
	_accumulating = false;
	_face = voxel::FaceNames::Max;
	_lastCursorPos = glm::ivec3(InvalidCursorCoord);
	_modifiedFlags = SceneModifiedFlags::All;
}

void PolygonLasso::abort(BrushContext &ctx) {
	if (_accumulating) {
		_path.clear();
		_overlayPoints.clear();
		_overlayClosable = false;
		_accumulating = false;
		_modifiedFlags = SceneModifiedFlags::All;
	}
}

void PolygonLasso::invalidate() {
	_accumulating = false;
	_path.clear();
	_overlayPoints.clear();
	_overlayClosable = false;
}

void PolygonLasso::popLastPathEntry() {
	if (!_path.empty()) {
		_path.pop();
	}
}

void PolygonLasso::buildOverlayPoints(const BrushContext &ctx) {
	_overlayPoints.clear();
	_overlayClosable = false;
	if (_path.empty()) {
		return;
	}
	const glm::mat4 model = _sceneManager != nullptr ? _sceneManager->worldMatrix() : glm::mat4(1.0f);
	auto toWorld = [&model](const glm::ivec3 &local) {
		// Center on the voxel so the overlay line runs through voxel centers.
		return glm::vec3(model * glm::vec4(glm::vec3(local) + 0.5f, 1.0f));
	};
	_overlayPoints.reserve(_path.size() + 1);
	for (const glm::ivec3 &vertex : _path) {
		_overlayPoints.push_back(toWorld(vertex));
	}
	// Append the live cursor as the rubber-band endpoint.
	const glm::ivec3 cursor = snapToGrid(ctx.cursorPosition, ctx.gridResolution);
	_overlayPoints.push_back(toWorld(cursor));
	if (_path.size() >= 3) {
		const glm::ivec3 &firstPt = _path[0];
		const int du = glm::abs(cursor[_uAxis] - firstPt[_uAxis]);
		const int dv = glm::abs(cursor[_vAxis] - firstPt[_vAxis]);
		_overlayClosable = du <= CloseThresholdVoxels && dv <= CloseThresholdVoxels;
	}
}

void PolygonLasso::update(const BrushContext &ctx, double nowSeconds) {
	if (!_accumulating) {
		return;
	}
	_lastCursorPos = ctx.cursorPosition;
	buildOverlayPoints(ctx);
}

bool PolygonLasso::wantBrushGizmo(const BrushContext &ctx) const {
	return _accumulating && !_overlayPoints.empty();
}

void PolygonLasso::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	state.operations = BrushGizmo_WorldPolyline;
	state.worldPolyline = &_overlayPoints;
	state.worldPolylineClosed = _overlayClosable;
}

bool PolygonLasso::beginBrush(const BrushContext &ctx, const AABBBrushState &state) {
	if (_accumulating) {
		const glm::ivec3 cursor = snapToGrid(ctx.cursorPosition, ctx.gridResolution);
		// Close the polygon when clicking near the first vertex
		if (_path.size() >= 3) {
			const glm::ivec3 &firstPt = _path[0];
			const int du = glm::abs(cursor[_uAxis] - firstPt[_uAxis]);
			const int dv = glm::abs(cursor[_vAxis] - firstPt[_vAxis]);
			if (du <= CloseThresholdVoxels && dv <= CloseThresholdVoxels) {
				_accumulating = false;
				_overlayPoints.clear();
				_overlayClosable = false;
				_modifiedFlags = SceneModifiedFlags::All;
				return true;
			}
		}
		// Add the next vertex to the polygon
		_path.push_back(cursor);
		buildOverlayPoints(ctx);
		_modifiedFlags = SceneModifiedFlags::NoUndo;
		return true;
	}
	// Start a new lasso polygon
	_path.clear();
	_overlayPoints.clear();
	_overlayClosable = false;
	_path.reserve(PathInitialReserve);
	_path.push_back(snapToGrid(ctx.cursorPosition, ctx.gridResolution));
	_face = ctx.cursorFace;
	ellipseAxes(_face, _uAxis, _vAxis);
	_faceAxisIdx = math::getIndexForAxis(voxel::faceToAxis(_face));
	_accumulating = true;
	buildOverlayPoints(ctx);
	_modifiedFlags = SceneModifiedFlags::NoUndo;
	return true;
}

voxel::Region PolygonLasso::calcRegion(const BrushContext &ctx, const AABBBrushState &state) const {
	// During accumulation nothing is written to the volume - the polygon is shown as a
	// viewport overlay. Returning an invalid region keeps the PreviewManager from
	// allocating a preview copy every frame.
	if (_accumulating) {
		return voxel::Region::InvalidRegion;
	}
	return ctx.targetVolumeRegion;
}

void PolygonLasso::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
							const voxel::Region &region, const AABBBrushState &state) {
	if (_face == voxel::FaceNames::Max || _path.empty()) {
		return;
	}
	// While accumulating, the in-progress polygon is drawn as a world-space overlay
	// (see wantBrushGizmo/brushGizmoState). Nothing is written to the volume until the
	// polygon is closed, so this is a no-op every frame/click during accumulation.
	if (_accumulating) {
		_modifiedFlags = SceneModifiedFlags::NoUndo;
		return;
	}

	// Polygon closed: apply selection to surface voxels inside the polygon.
	_modifiedFlags = SceneModifiedFlags::All;
	if (_path.size() < 3) {
		return;
	}
	const int uAxis = _uAxis;
	const int vAxis = _vAxis;
	const int wAxis = _faceAxisIdx;
	const bool positiveNormal = voxel::isPositiveFace(_face);
	auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &voxel) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};
	// Flood-fill the front surface inside the polygon: seed along the drawn edges, then expand
	// across (u,v) cells staying on the same surface (depth continuity within _depthTolerance).
	// This keeps the selection on the visible skin instead of wrapping down side walls, and
	// still skips disjoint structures that merely share the (u, v) silhouette.
	voxelutil::lassoFloodFillSurface(wrapper, _path, uAxis, vAxis, wAxis, positiveNormal, ctx.targetVolumeRegion, func,
									 _depthTolerance);
	_path.clear();
}

} // namespace select
} // namespace voxedit
