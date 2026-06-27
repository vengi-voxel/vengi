/**
 * @file
 */

#include "Rectangle.h"
#include "Circle.h"
#include "math/Axis.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/BrushGizmo.h"
#include "voxel/Face.h"
#include "voxelutil/VolumeSelect.h"
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>

namespace voxedit {
namespace select {

void Rectangle::computeCorners(const voxel::Region &region, voxel::FaceNames face, glm::ivec3 corners[4]) const {
	int uAxis;
	int vAxis;
	Circle::ellipseAxes(face, uAxis, vAxis);
	const int wAxis = math::getIndexForAxis(voxel::faceToAxis(face));
	const bool positiveNormal = voxel::isPositiveFace(face);
	const glm::ivec3 mins = region.getLowerCorner();
	const glm::ivec3 maxs = region.getUpperCorner();
	const float cu = (float)(mins[uAxis] + maxs[uAxis]) * 0.5f;
	const float cv = (float)(mins[vAxis] + maxs[vAxis]) * 0.5f;
	const float hu = (float)(maxs[uAxis] - mins[uAxis]) * 0.5f;
	const float hv = (float)(maxs[vAxis] - mins[vAxis]) * 0.5f;
	// Anchor the outline at the clicked (near) face depth.
	const int w0 = positiveNormal ? maxs[wAxis] : mins[wAxis];
	const float rad = glm::radians((float)_angle);
	const float cs = glm::cos(rad);
	const float sn = glm::sin(rad);
	const float local[4][2] = {{hu, hv}, {-hu, hv}, {-hu, -hv}, {hu, -hv}};
	for (int i = 0; i < 4; ++i) {
		const float lu = local[i][0];
		const float lv = local[i][1];
		glm::ivec3 p(0);
		p[uAxis] = (int)glm::round(cu + lu * cs - lv * sn);
		p[vAxis] = (int)glm::round(cv + lu * sn + lv * cs);
		p[wAxis] = w0;
		corners[i] = p;
	}
}

void Rectangle::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						 const voxel::Region &region, const AABBBrushState &state) {
	if (state.aabbFace == voxel::FaceNames::Max) {
		return;
	}
	int uAxis;
	int vAxis;
	Circle::ellipseAxes(state.aabbFace, uAxis, vAxis);
	const int wAxis = math::getIndexForAxis(voxel::faceToAxis(state.aabbFace));
	const bool positiveNormal = voxel::isPositiveFace(state.aabbFace);

	glm::ivec3 corners[4];
	computeCorners(region, state.aabbFace, corners);

	// Snap each corner to the actual surface depth at its (u,v) so the edge chord follows the
	// surface across terraces instead of staying pinned to the single clicked depth (which left
	// the down-slope edges broken at every riser).
	const voxel::Region &vol = ctx.targetVolumeRegion;
	const int w0 = corners[0][wAxis];
	const int columnTol = vol.getUpperCorner()[wAxis] - vol.getLowerCorner()[wAxis];
	for (int i = 0; i < 4; ++i) {
		int sw;
		if (voxelutil::findSurfaceNear(wrapper, corners[i][uAxis], corners[i][vAxis], w0, columnTol, uAxis, vAxis, wAxis,
									   positiveNormal, vol, sw)) {
			corners[i][wAxis] = sw;
		}
	}

	auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};
	// Draw the four edges of the (rotated) rectangle as lines draped over the visible surface, so
	// each edge stays continuous AND visible across terraces/risers (riser faces are filled in).
	for (int i = 0; i < 4; ++i) {
		voxelutil::lineDrapeSurface(wrapper, corners[i], corners[(i + 1) % 4], uAxis, vAxis, wAxis, positiveNormal,
									_edgeWidth, vol, func);
	}
}

void Rectangle::reset() {
	Super::reset();
	_previewActive = false;
	_overlayPoints.clear();
}

void Rectangle::abort(BrushContext &ctx) {
	Super::abort(ctx);
	_previewActive = false;
	_overlayPoints.clear();
}

void Rectangle::endBrush(BrushContext &ctx) {
	Super::endBrush(ctx);
	_previewActive = false;
	_overlayPoints.clear();
}

void Rectangle::setPreview(const voxel::Region &region, voxel::FaceNames face) {
	if (face == voxel::FaceNames::Max || !region.isValid()) {
		_previewActive = false;
		_overlayPoints.clear();
		return;
	}
	glm::ivec3 corners[4];
	computeCorners(region, face, corners);
	const glm::mat4 model = _sceneManager != nullptr ? _sceneManager->worldMatrix() : glm::mat4(1.0f);
	_overlayPoints.clear();
	_overlayPoints.reserve(4);
	for (int i = 0; i < 4; ++i) {
		// Center on the voxel so the outline runs through voxel centers.
		_overlayPoints.push_back(glm::vec3(model * glm::vec4(glm::vec3(corners[i]) + 0.5f, 1.0f)));
	}
	_previewActive = true;
}

bool Rectangle::wantBrushGizmo(const BrushContext &ctx) const {
	return _previewActive && _overlayPoints.size() == 4;
}

void Rectangle::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	state.operations = BrushGizmo_WorldPolyline;
	state.worldPolyline = &_overlayPoints;
	state.worldPolylineClosed = true;
}

} // namespace select
} // namespace voxedit
