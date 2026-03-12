/**
 * @file
 */

#include "SelectBrush.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include "palette/Palette.h"
#include <glm/geometric.hpp>

namespace voxedit {

void SelectBrush::ellipseAxes(voxel::FaceNames face, int &uAxis, int &vAxis) {
	const math::Axis axis = voxel::faceToAxis(face);
	const int faceAxisIdx = math::getIndexForAxis(axis);
	// The two perpendicular axes in order
	uAxis = (faceAxisIdx + 1) % 3;
	vAxis = (faceAxisIdx + 2) % 3;
}

bool SelectBrush::insideSelection(const glm::ivec3 &pos, const glm::ivec3 &center,
								  int radiusU, int radiusV, int depth, bool is3D,
								  int uAxis, int vAxis, int faceAxisIdx, bool positiveNormal) {
	if (radiusU <= 0 || radiusV <= 0) {
		return false;
	}
	// Signed depth: how far behind the surface the position is
	// For positive faces, "behind" is the negative direction; for negative faces, positive
	const int dd = positiveNormal ? (center[faceAxisIdx] - pos[faceAxisIdx])
								  : (pos[faceAxisIdx] - center[faceAxisIdx]);
	if (dd < 0) {
		return false;
	}
	if (is3D && depth > 0) {
		const double du = (double)(pos[uAxis] - center[uAxis]);
		const double dv = (double)(pos[vAxis] - center[vAxis]);
		const double ddd = (double)dd;
		return (du * du) / ((double)radiusU * radiusU) +
			   (dv * dv) / ((double)radiusV * radiusV) +
			   (ddd * ddd) / ((double)depth * depth) <= 1.0;
	}
	// 2D ellipse + flat depth check (also used as fallback when 3D with depth=0)
	if (!insideEllipse(pos, center, radiusU, radiusV, uAxis, vAxis)) {
		return false;
	}
	return dd <= depth;
}

bool SelectBrush::insideEllipse(const glm::ivec3 &pos, const glm::ivec3 &center,
								int radiusU, int radiusV, int uAxis, int vAxis) {
	if (radiusU <= 0 || radiusV <= 0) {
		return false;
	}
	const double du = (double)(pos[uAxis] - center[uAxis]);
	const double dv = (double)(pos[vAxis] - center[vAxis]);
	return (du * du) / ((double)radiusU * radiusU) + (dv * dv) / ((double)radiusV * radiusV) <= 1.0;
}

bool SelectBrush::needsAdditionalAction(const BrushContext &ctx) const {
	// Circle, Connected, SameColor, Surface, FuzzyColor, FlatSurface use their own
	// selection logic -they don't benefit from the 3-click AABB workflow.
	// Only All and Box3D use the standard AABB region.
	if (_selectMode != SelectMode::All && _selectMode != SelectMode::Box3D) {
		return false;
	}
	return Super::needsAdditionalAction(ctx);
}

bool SelectBrush::beginBrush(const BrushContext &ctx) {
	if (_selectMode == SelectMode::Circle) {
		_ellipseValid = false;
		_ellipseHistory.clear();
	}
	return Super::beginBrush(ctx);
}

voxel::Region SelectBrush::calcRegion(const BrushContext &ctx) const {
	if (_selectMode == SelectMode::Circle && _aabbMode && _aabbFace != voxel::FaceNames::Max) {
		const glm::ivec3 center = applyGridResolution(_aabbFirstPos, ctx.gridResolution);
		const glm::ivec3 current = currentCursorPosition(ctx);
		int uAxis;
		int vAxis;
		ellipseAxes(_aabbFace, uAxis, vAxis);
		const int du = glm::abs(current[uAxis] - center[uAxis]);
		const int dv = glm::abs(current[vAxis] - center[vAxis]);
		const int r = glm::max(du, dv);
		glm::ivec3 mins = center;
		glm::ivec3 maxs = center;
		mins[uAxis] -= r;
		maxs[uAxis] += r;
		mins[vAxis] -= r;
		maxs[vAxis] += r;
		// Limit face-normal axis by depth -only extend behind the surface
		const math::Axis faceAxis = voxel::faceToAxis(_aabbFace);
		const int faceAxisIdx = math::getIndexForAxis(faceAxis);
		if (voxel::isPositiveFace(_aabbFace)) {
			// Behind a positive face = negative direction
			mins[faceAxisIdx] = center[faceAxisIdx] - _ellipseDepth;
			maxs[faceAxisIdx] = center[faceAxisIdx];
		} else {
			// Behind a negative face = positive direction
			mins[faceAxisIdx] = center[faceAxisIdx];
			maxs[faceAxisIdx] = center[faceAxisIdx] + _ellipseDepth;
		}
		voxel::Region circleRegion(mins, maxs);
		circleRegion.cropTo(ctx.targetVolumeRegion);
		return circleRegion;
	}
	// Only All and Box3D use the standard AABB region; all other modes
	// flood-fill or visit the full volume and don't need a user-drawn box.
	if (_selectMode != SelectMode::All && _selectMode != SelectMode::Box3D) {
		return ctx.targetVolumeRegion;
	}
	return Super::calcRegion(ctx);
}

void SelectBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						   const voxel::Region &region) {
	voxel::Region selectionRegion = region;
	if (_brushClamping) {
		selectionRegion.cropTo(ctx.targetVolumeRegion);
	}

	// Clear box region by default; Box3D case sets it below
	wrapper.node().setSelectionRegion(voxel::Region::InvalidRegion);

	auto func = [&wrapper](int x, int y, int z, const voxel::Voxel &voxel) {
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(x, y, z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(x, y, z, voxel::FlagOutline);
		}
	};

	switch (_selectMode) {
	case SelectMode::All: {
		voxelutil::VisitSolid condition;
		voxelutil::visitVolumeParallel(wrapper, selectionRegion, func, condition);
		break;
	}
	case SelectMode::Surface: {
		voxelutil::visitSurfaceVolumeParallel(wrapper, func);
		break;
	}
	case SelectMode::SameColor: {
		const voxel::Voxel &referenceVoxel = ctx.hitCursorVoxel;
		if (voxel::isAir(referenceVoxel.getMaterial())) {
			return;
		}
		voxelutil::VisitVoxelColor condition = voxelutil::VisitVoxelColor(referenceVoxel);
		voxelutil::visitVolumeParallel(wrapper, selectionRegion, func, condition);
		break;
	}
	case SelectMode::FuzzyColor: {
		const voxel::Voxel &referenceVoxel = ctx.hitCursorVoxel;
		if (voxel::isAir(referenceVoxel.getMaterial())) {
			return;
		}
		const palette::Palette &palette = wrapper.node().palette();
		voxelutil::VisitVoxelFuzzyColor condition(palette, referenceVoxel.getColor(), _colorThreshold);
		voxelutil::visitVolumeParallel(wrapper, selectionRegion, func, condition);
		break;
	}
	case SelectMode::Connected: {
		const voxel::Voxel &referenceVoxel = ctx.hitCursorVoxel;
		if (voxel::isAir(referenceVoxel.getMaterial())) {
			return;
		}
		const glm::ivec3 &startPos = ctx.cursorPosition;
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.removeFlagAt(startPos.x, startPos.y, startPos.z, voxel::FlagOutline);
		} else {
			wrapper.setFlagAt(startPos.x, startPos.y, startPos.z, voxel::FlagOutline);
		}
		voxelutil::visitConnectedByCondition(wrapper, startPos, func);
		break;
	}
	case SelectMode::FlatSurface: {
		if (ctx.cursorFace == voxel::FaceNames::Max) {
			return;
		}
		const glm::ivec3 &startPos = ctx.cursorPosition;
		if (voxel::isAir(wrapper.voxel(startPos).getMaterial())) {
			return;
		}
		voxelutil::visitFlatSurface(wrapper, startPos, ctx.cursorFace, _flatDeviation, func);
		break;
	}
	case SelectMode::Circle: {
		// Use _aabbFace (the face from the initial click) rather than ctx.cursorFace
		// which can change during the drag as the cursor moves across faces
		const voxel::FaceNames face = _aabbFace;
		if (face == voxel::FaceNames::Max) {
			return;
		}
		const glm::ivec3 center(_aabbFirstPos);
		int uAxis;
		int vAxis;
		ellipseAxes(face, uAxis, vAxis);
		const int faceAxisIdx = math::getIndexForAxis(voxel::faceToAxis(face));
		// During initial drag, both radii are the same (circle), depth defaults to 1
		const int du = glm::abs(ctx.cursorPosition[uAxis] - center[uAxis]);
		const int dv = glm::abs(ctx.cursorPosition[vAxis] - center[vAxis]);
		const int radius = glm::max(du, dv);
		const int radiusU = radius;
		const int radiusV = radius;
		const int depth = _ellipseDepth;
		const bool is3D = _ellipse3D;
		const bool positiveNormal = voxel::isPositiveFace(face);
		auto inBounds = [&](int x, int y, int z) {
			const glm::ivec3 pos(x, y, z);
			return insideSelection(pos, center, radiusU, radiusV, depth, is3D,
								   uAxis, vAxis, faceAxisIdx, positiveNormal);
		};
		if (_previewMode) {
			// In preview mode, remove voxels outside the selection from the preview copy
			// so the ghost overlay shows only the selection shape
			voxel::RawVolume *vol = wrapper.volume();
			const voxel::Region &r = vol->region();
			const voxel::Voxel air;
			for (int z = r.getLowerZ(); z <= r.getUpperZ(); ++z) {
				for (int y = r.getLowerY(); y <= r.getUpperY(); ++y) {
					for (int x = r.getLowerX(); x <= r.getUpperX(); ++x) {
						if (!inBounds(x, y, z)) {
							vol->setVoxel(x, y, z, air);
						}
					}
				}
			}
		} else {
			_ellipseHistory.clear();
			auto circleFunc = [&](int x, int y, int z, const voxel::Voxel &v) {
				if (inBounds(x, y, z)) {
					func(x, y, z, v);
					_ellipseHistory.push_back(glm::ivec3(x, y, z));
				}
			};
			if (depth > 1) {
				// Visit all solid voxels in the ellipse bounding region.
				// visitSurfaceVolume would skip interior voxels.
				voxelutil::VisitSolid condition;
				voxelutil::visitVolume(wrapper, selectionRegion, circleFunc, condition);
			} else {
				voxelutil::visitSurfaceVolume(wrapper, circleFunc);
			}
			// Cache ellipse parameters for slider adjustment
			_ellipseCenter = center;
			_ellipseRadiusU = radiusU;
			_ellipseRadiusV = radiusV;
			_ellipseDepth = depth;
			_ellipse3D = is3D;
			_ellipseFace = face;
			_ellipseValid = true;
		}
		break;
	}
	case SelectMode::Slope: {
		if (ctx.cursorFace == voxel::FaceNames::Max) {
			return;
		}
		const glm::ivec3 &startPos = ctx.cursorPosition;
		if (voxel::isAir(wrapper.voxel(startPos).getMaterial())) {
			return;
		}
		_slopeHistory.clear();
		auto slopeFunc = [&](int x, int y, int z, const voxel::Voxel &v) {
			func(x, y, z, v);
			_slopeHistory.push_back(glm::ivec3(x, y, z));
		};
		voxelutil::visitSlopeSurface(wrapper, startPos, ctx.cursorFace, _slopeDeviation, _slopeSampleDistance, slopeFunc);
		_slopeSeedPos = startPos;
		_slopeFace = ctx.cursorFace;
		_slopeValid = true;
		break;
	}
	case SelectMode::Box3D: {
		wrapper.removeFlags(wrapper.region(), voxel::FlagOutline);
		voxelutil::VisitSolid condition;
		voxelutil::visitVolumeParallel(wrapper, selectionRegion, func, condition);
		// Store the exact box region so ModifierVolumeWrapper::skip() allows
		// editing any position inside the box (including air voxels)
		if (wrapper.modifierType() == ModifierType::Erase) {
			wrapper.node().setSelectionRegion(voxel::Region::InvalidRegion);
		} else {
			wrapper.node().setSelectionRegion(selectionRegion);
		}
		break;
	}
	case SelectMode::Max:
		return;
	}
}

} // namespace voxedit
