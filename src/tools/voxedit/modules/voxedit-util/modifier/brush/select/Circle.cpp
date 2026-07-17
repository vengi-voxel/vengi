/**
 * @file
 */

#include "Circle.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Face.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {
namespace select {

void Circle::ellipseAxes(voxel::FaceNames face, int &uAxis, int &vAxis) {
	const math::Axis axis = voxel::faceToAxis(face);
	const int faceAxisIdx = math::getIndexForAxis(axis);
	uAxis = (faceAxisIdx + 1) % 3;
	vAxis = (faceAxisIdx + 2) % 3;
}

bool Circle::insideEllipse(const glm::ivec3 &pos, const glm::ivec3 &center, int radiusU, int radiusV, int uAxis,
						   int vAxis) {
	if (radiusU <= 0 || radiusV <= 0) {
		return false;
	}
	const long long du = (long long)(pos[uAxis] - center[uAxis]);
	const long long dv = (long long)(pos[vAxis] - center[vAxis]);
	const long long rU2 = (long long)radiusU * (long long)radiusU;
	const long long rV2 = (long long)radiusV * (long long)radiusV;
	return du * du * rV2 + dv * dv * rU2 <= rU2 * rV2;
}

bool Circle::insideSelection(const glm::ivec3 &pos, const glm::ivec3 &center, int radiusU, int radiusV, int depth,
							 bool is3D, int uAxis, int vAxis, int faceAxisIdx, bool positiveNormal) {
	if (radiusU <= 0 || radiusV <= 0) {
		return false;
	}
	const int dd = positiveNormal ? (center[faceAxisIdx] - pos[faceAxisIdx]) : (pos[faceAxisIdx] - center[faceAxisIdx]);
	if (dd < 0) {
		return false;
	}
	if (is3D && depth > 0) {
		const long long du = (long long)(pos[uAxis] - center[uAxis]);
		const long long dv = (long long)(pos[vAxis] - center[vAxis]);
		const long long ddd = (long long)dd;
		const long long rU2 = (long long)radiusU * (long long)radiusU;
		const long long rV2 = (long long)radiusV * (long long)radiusV;
		const long long d2 = (long long)depth * (long long)depth;
		return du * du * rV2 * d2 + dv * dv * rU2 * d2 + ddd * ddd * rU2 * rV2 <= rU2 * rV2 * d2;
	}
	if (!insideEllipse(pos, center, radiusU, radiusV, uAxis, vAxis)) {
		return false;
	}
	return dd <= depth;
}

bool Circle::beginBrush(const BrushContext &ctx, const AABBBrushState &state) {
	invalidate();
	return false;
}

void Circle::reset() {
	invalidate();
}

void Circle::invalidate() {
	_ellipseValid = false;
	_ellipseHistory.clear();
}

voxel::Region Circle::calcRegion(const BrushContext &ctx, const AABBBrushState &state) const {
	if (!state.boxMode || state.aabbFace == voxel::FaceNames::Max) {
		return voxel::Region::InvalidRegion;
	}
	const glm::ivec3 center = state.aabbFirstPos;
	const glm::ivec3 current = state.cursorPosition;
	int uAxis;
	int vAxis;
	ellipseAxes(state.aabbFace, uAxis, vAxis);
	const int du = glm::abs(current[uAxis] - center[uAxis]);
	const int dv = glm::abs(current[vAxis] - center[vAxis]);
	const int r = glm::max(du, dv);
	glm::ivec3 mins = center;
	glm::ivec3 maxs = center;
	mins[uAxis] -= r;
	maxs[uAxis] += r;
	mins[vAxis] -= r;
	maxs[vAxis] += r;
	const math::Axis faceAxis = voxel::faceToAxis(state.aabbFace);
	const int faceAxisIdx = math::getIndexForAxis(faceAxis);
	if (voxel::isPositiveFace(state.aabbFace)) {
		mins[faceAxisIdx] = center[faceAxisIdx] - _ellipseDepth;
		maxs[faceAxisIdx] = center[faceAxisIdx];
	} else {
		mins[faceAxisIdx] = center[faceAxisIdx];
		maxs[faceAxisIdx] = center[faceAxisIdx] + _ellipseDepth;
	}
	voxel::Region circleRegion(mins, maxs);
	circleRegion.cropTo(ctx.targetVolumeRegion);
	return circleRegion;
}

void Circle::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					  const voxel::Region &region, const AABBBrushState &state) {
	if (state.aabbFace == voxel::FaceNames::Max) {
		return;
	}
	const glm::ivec3 center(state.aabbFirstPos);
	int uAxis;
	int vAxis;
	ellipseAxes(state.aabbFace, uAxis, vAxis);
	const int faceAxisIdx = math::getIndexForAxis(voxel::faceToAxis(state.aabbFace));
	const int du = glm::abs(ctx.cursorPosition[uAxis] - center[uAxis]);
	const int dv = glm::abs(ctx.cursorPosition[vAxis] - center[vAxis]);
	const int radius = glm::max(du, dv);
	const int radiusU = radius;
	const int radiusV = radius;
	const int depth = _ellipseDepth;
	const bool is3D = _ellipse3D;
	const bool positiveNormal = voxel::isPositiveFace(state.aabbFace);
	auto inBounds = [&](int x, int y, int z) {
		const glm::ivec3 pos(x, y, z);
		return insideSelection(pos, center, radiusU, radiusV, depth, is3D, uAxis, vAxis, faceAxisIdx, positiveNormal);
	};
	_ellipseHistory.clear();
	_ellipseHistory.reserve(region.voxels());
	auto circleFunc = [&](int x, int y, int z, const voxel::Voxel &v) {
		if (inBounds(x, y, z)) {
			if (wrapper.modifierType() == ModifierType::Erase) {
				wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
			} else {
				wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
			}
			_ellipseHistory.push_back(glm::ivec3(x, y, z));
		}
	};
	if (depth > 1) {
		voxelutil::VisitSolid condition;
		voxelutil::visitVolume(wrapper, region, circleFunc, condition);
	} else {
		voxelutil::VisitVisible condition;
		voxelutil::visitVolume(wrapper, region, circleFunc, condition);
	}
	_ellipseCenter = center;
	_ellipseRadiusU = radiusU;
	_ellipseRadiusV = radiusV;
	_ellipseDepth = depth;
	_ellipse3D = is3D;
	_ellipseFace = state.aabbFace;
	_ellipseValid = true;
}

} // namespace select
} // namespace voxedit
