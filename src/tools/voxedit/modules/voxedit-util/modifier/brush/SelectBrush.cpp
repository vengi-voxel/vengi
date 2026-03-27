/**
 * @file
 */

#include "SelectBrush.h"
#include "core/collection/DynamicMap.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeSelect.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/geometric.hpp>

namespace voxedit {

void SelectBrush::ellipseAxes(voxel::FaceNames face, int &uAxis, int &vAxis) {
	const math::Axis axis = voxel::faceToAxis(face);
	const int faceAxisIdx = math::getIndexForAxis(axis);
	// The two perpendicular axes in order
	uAxis = (faceAxisIdx + 1) % 3;
	vAxis = (faceAxisIdx + 2) % 3;
}

bool SelectBrush::insideSelection(const glm::ivec3 &pos, const glm::ivec3 &center, int radiusU, int radiusV, int depth,
								  bool is3D, int uAxis, int vAxis, int faceAxisIdx, bool positiveNormal) {
	if (radiusU <= 0 || radiusV <= 0) {
		return false;
	}
	// Signed depth: how far behind the surface the position is
	// For positive faces, "behind" is the negative direction; for negative faces, positive
	const int dd = positiveNormal ? (center[faceAxisIdx] - pos[faceAxisIdx]) : (pos[faceAxisIdx] - center[faceAxisIdx]);
	if (dd < 0) {
		return false;
	}
	if (is3D && depth > 0) {
		const double du = (double)(pos[uAxis] - center[uAxis]);
		const double dv = (double)(pos[vAxis] - center[vAxis]);
		const double ddd = (double)dd;
		return (du * du) / ((double)radiusU * radiusU) + (dv * dv) / ((double)radiusV * radiusV) +
				   (ddd * ddd) / ((double)depth * depth) <=
			   1.0;
	}
	// 2D ellipse + flat depth check (also used as fallback when 3D with depth=0)
	if (!insideEllipse(pos, center, radiusU, radiusV, uAxis, vAxis)) {
		return false;
	}
	return dd <= depth;
}

bool SelectBrush::insideEllipse(const glm::ivec3 &pos, const glm::ivec3 &center, int radiusU, int radiusV, int uAxis,
								int vAxis) {
	if (radiusU <= 0 || radiusV <= 0) {
		return false;
	}
	const double du = (double)(pos[uAxis] - center[uAxis]);
	const double dv = (double)(pos[vAxis] - center[vAxis]);
	return (du * du) / ((double)radiusU * radiusU) + (dv * dv) / ((double)radiusV * radiusV) <= 1.0;
}

bool SelectBrush::active() const {
	if (_selectMode == SelectMode::Lasso && _lassoAccumulating) {
		return true;
	}
	return Super::active();
}

void SelectBrush::reset() {
	Super::reset();
	_lassoPath.clear();
	_lassoEdgeHistory.clear();
	_lassoAccumulating = false;
	_paintAccumulating = false;
	_paintDirtyRegion = voxel::Region::InvalidRegion;
	_sceneModifiedFlags = SceneModifiedFlags::All;
	_paintFinalUndoRegion = voxel::Region::InvalidRegion;
}

void SelectBrush::onSceneChange() {
	Super::onSceneChange();
	_lassoPath.clear();
	_lassoEdgeHistory.clear();
	_lassoAccumulating = false;
	_paintAccumulating = false;
	_paintDirtyRegion = voxel::Region::InvalidRegion;
	_sceneModifiedFlags = SceneModifiedFlags::All;
}

void SelectBrush::abort(BrushContext &ctx) {
	if (_selectMode == SelectMode::Lasso && _lassoAccumulating) {
		_lassoPath.clear();
		_lassoAccumulating = false;
		_sceneModifiedFlags = SceneModifiedFlags::All;
	}
	if (_selectMode == SelectMode::Paint && _paintAccumulating) {
		_paintAccumulating = false;
		_paintDirtyRegion = voxel::Region::InvalidRegion;
		_sceneModifiedFlags = SceneModifiedFlags::All;
	}
	Super::abort(ctx);
}

bool SelectBrush::hasPendingChanges() const {
	if (_lassoAccumulating && !_lassoEdgeHistory.empty()) {
		return true;
	}
	if (_paintAccumulating && _paintDirtyRegion.isValid()) {
		return true;
	}
	return false;
}

voxel::Region SelectBrush::revertChanges(voxel::RawVolume *volume) {
	voxel::Region dirtyRegion = voxel::Region::InvalidRegion;
	for (const voxel::VoxelPosition &entry : _lassoEdgeHistory) {
		volume->setVoxel(entry.pos, entry.voxel);
		dirtyRegion.accumulate(entry.pos);
	}
	_lassoEdgeHistory.clear();
	return dirtyRegion;
}

void SelectBrush::endBrush(BrushContext &ctx) {
	if (_selectMode == SelectMode::Paint && _paintAccumulating) {
		_paintAccumulating = false;
		_paintFinalUndoRegion = _paintDirtyRegion;
		_paintDirtyRegion = voxel::Region::InvalidRegion;
		_sceneModifiedFlags = SceneModifiedFlags::All;
	}
	Super::endBrush(ctx);
}

voxel::Region SelectBrush::consumePendingUndoRegion() {
	voxel::Region region = _paintFinalUndoRegion;
	_paintFinalUndoRegion = voxel::Region::InvalidRegion;
	return region;
}

void SelectBrush::update(const BrushContext &ctx, double nowSeconds) {
	Super::update(ctx, nowSeconds);
	// Between clicks in lasso polygon mode, _aabbMode is false but the brush stays
	// active via active(). Trigger preview refresh when the cursor moves so the
	// rubber-band preview line updates in real time.
	if (_selectMode == SelectMode::Lasso && _lassoAccumulating && !_aabbMode) {
		if (ctx.cursorPosition != _lastLassoCursorPos) {
			_lastLassoCursorPos = ctx.cursorPosition;
			markDirty();
		}
	}
}

void SelectBrush::setSelectMode(SelectMode mode) {
	if (_selectMode != mode) {
		if (_selectMode == SelectMode::Paint) {
			setAABBMode();
			_paintAccumulating = false;
			_paintDirtyRegion = voxel::Region::InvalidRegion;
			_sceneModifiedFlags = SceneModifiedFlags::All;
		}
		_ellipseValid = false;
		_slopeValid = false;
		_lassoAccumulating = false;
		_lassoPath.clear();
		// Edge history voxels on the real volume become orphaned here.
		// The user can undo to recover them; clearing avoids stale state.
		_lassoEdgeHistory.clear();
		if (mode == SelectMode::Paint) {
			setSingleMode();
			if (_radius == 0) {
				setRadius(1);
			}
		}
	}
	_selectMode = mode;
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
	if (_selectMode == SelectMode::Lasso) {
		if (_lassoAccumulating) {
			const glm::ivec3 &cursor = applyGridResolution(ctx.cursorPosition, ctx.gridResolution);
			// Close the polygon when clicking near the first vertex
			if (_lassoPath.size() >= 3) {
				const glm::ivec3 &firstPt = _lassoPath[0];
				const int du = glm::abs(cursor[_lassoUAxis] - firstPt[_lassoUAxis]);
				const int dv = glm::abs(cursor[_lassoVAxis] - firstPt[_lassoVAxis]);
				if (du <= LassoCloseThresholdVoxels && dv <= LassoCloseThresholdVoxels) {
					_lassoAccumulating = false;
					_aabbFace = ctx.cursorFace;
					_aabbMode = true;
					return true;
				}
			}
			// Add the next vertex to the polygon
			_lassoPath.push_back(cursor);
			_aabbFace = ctx.cursorFace;
			_aabbMode = true;
			return true;
		}
		// Start a new lasso polygon
		_lassoPath.clear();
		_lassoEdgeHistory.clear();
		_lassoPath.reserve(LassoPathInitialReserve);
		_lassoPath.push_back(applyGridResolution(ctx.cursorPosition, ctx.gridResolution));
		_lassoFace = ctx.cursorFace;
		ellipseAxes(_lassoFace, _lassoUAxis, _lassoVAxis);
		_lassoFaceAxisIdx = math::getIndexForAxis(voxel::faceToAxis(_lassoFace));
		_lassoAccumulating = true;
		_aabbFace = ctx.cursorFace;
		_aabbMode = true;
		return true;
	}
	if (_selectMode == SelectMode::Circle) {
		_ellipseValid = false;
		_ellipseHistory.clear();
	}
	if (_selectMode == SelectMode::Paint) {
		_paintAccumulating = true;
		_paintDirtyRegion = voxel::Region::InvalidRegion;
		_paintFinalUndoRegion = voxel::Region::InvalidRegion;
		_sceneModifiedFlags = SceneModifiedFlags::NoUndo;
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
	// For lasso accumulation, return a tight bbox of the rubber-band segment
	// (last vertex to cursor) so the preview system can show it as a ghost.
	if (_selectMode == SelectMode::Lasso && _lassoAccumulating && !_lassoPath.empty()) {
		const glm::ivec3 &lastPt = _lassoPath.back();
		const glm::ivec3 &cursor = ctx.cursorPosition;
		const glm::ivec3 mins = glm::min(lastPt, cursor);
		const glm::ivec3 maxs = glm::max(lastPt, cursor);
		voxel::Region rubberBandRegion(mins, maxs);
		rubberBandRegion.cropTo(ctx.targetVolumeRegion);
		return rubberBandRegion;
	}
	// All, Box3D, and Paint use the parent's region calculation.
	// All other modes flood-fill or visit the full volume.
	if (_selectMode != SelectMode::All && _selectMode != SelectMode::Box3D && _selectMode != SelectMode::Paint) {
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
			return insideSelection(pos, center, radiusU, radiusV, depth, is3D, uAxis, vAxis, faceAxisIdx,
								   positiveNormal);
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
		voxelutil::visitSlopeSurface(wrapper, startPos, ctx.cursorFace, _slopeDeviation, _slopeSampleDistance,
									 slopeFunc);
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
	case SelectMode::Lasso: {
		if (_lassoFace == voxel::FaceNames::Max || _lassoPath.empty()) {
			return;
		}
		const int uAxis = _lassoUAxis;
		const int vAxis = _lassoVAxis;
		const int wAxis = _lassoFaceAxisIdx;
		const bool positiveNormal = voxel::isPositiveFace(_lassoFace);

		// Restore all edge history voxels into the wrapper, adding them to the dirty
		// region so the NoUndo edge marks are included in any subsequent undo entry.
		auto restoreEdgeHistory = [&]() {
			for (const voxel::VoxelPosition &entry : _lassoEdgeHistory) {
				wrapper.volume()->setVoxel(entry.pos, entry.voxel);
				wrapper.addToDirtyRegion(entry.pos);
			}
			_lassoEdgeHistory.clear();
		};

		if (_lassoAccumulating) {
			const int vertexCount = (int)_lassoPath.size();
			if (_previewMode) {
				// Preview copy already contains edge marks from the real volume.
				// Draw only the rubber-band from last vertex to current cursor.
				if (ctx.cursorFace != voxel::FaceNames::Max && vertexCount >= 1) {
					voxelutil::drawLassoEdgeSurface([&](const glm::ivec3 &pos) { return wrapper.voxel(pos); },
													_lassoPath[vertexCount - 1], ctx.cursorPosition, uAxis, vAxis,
													wAxis, positiveNormal, selectionRegion, func);
				}
			} else {
				// Draw committed edges on the real volume (NoUndo - no undo entry per click).
				// Restore previous marks first so stale selections don't linger.
				restoreEdgeHistory();
				// Only store the original voxel on first encounter - if two edge segments
				// share a position, the first visit has the unmodified voxel state.
				auto edgeFunc = [&](int x, int y, int z, const voxel::Voxel &v) {
					if (!_lassoEdgeHistory.hasVoxel(glm::ivec3(x, y, z))) {
						_lassoEdgeHistory.setVoxel(x, y, z, v);
					}
					func(x, y, z, v);
				};
				for (int edgeIdx = 1; edgeIdx < vertexCount; ++edgeIdx) {
					voxelutil::drawLassoEdgeSurface([&](const glm::ivec3 &pos) { return wrapper.voxel(pos); },
													_lassoPath[edgeIdx - 1], _lassoPath[edgeIdx], uAxis, vAxis, wAxis,
													positiveNormal, selectionRegion, edgeFunc);
				}
				_sceneModifiedFlags = SceneModifiedFlags::NoUndo;
			}
			return;
		}

		// Polygon closed: apply selection to surface voxels inside the polygon.
		_sceneModifiedFlags = SceneModifiedFlags::All;
		if (_lassoPath.size() < 3) {
			return;
		}
		if (!_previewMode) {
			// Restore edge history voxels BEFORE applying selection so their positions
			// are included in the dirty region. This ensures the undo entry covers ALL
			// edge marks (including those outside the polygon) and undo reverts cleanly.
			restoreEdgeHistory();
		}
		auto lassoFunc = [&](int x, int y, int z, const voxel::Voxel &v) {
			const glm::ivec3 pos(x, y, z);
			if (voxelutil::lassoContains(_lassoPath, pos[uAxis], pos[vAxis], uAxis, vAxis)) {
				func(x, y, z, v);
			}
		};
		voxelutil::visitSurfaceVolume(wrapper, lassoFunc);
		if (!_previewMode) {
			_lassoPath.clear();
		}
		break;
	}
	case SelectMode::HoleRim2D: {
		// Select the closed rim of a hole in an axis-aligned (coplanar) surface.
		//
		// Two types of air seeds are tried:
		//   1. The 4 UV neighbors of the clicked voxel at the same W depth. Handles
		//      clicking the face that lies flat on the surface containing the hole
		//      (e.g. top face for a well opening: air is beside the rim in the floor
		//      plane, and the BFS in XZ stays bounded).
		//   2. The voxel one step in the face direction (the air the face "looks into").
		//      All three possible axis planes through that air voxel are tried, because
		//      which plane is correct depends on the geometry, not on the clicked face.
		//      Example: a side-face click on a well rim puts the air seed inside the
		//      hole; only the floor plane (XZ at Y=0) is bounded - the YZ and XY planes
		//      both reach open exterior air. The correct plane is found automatically.
		//
		// processedAir is updated ONLY after a successful bounded BFS so that a failed
		// (unbounded) attempt does not block other axis planes from retrying the same
		// air seed.
		const voxel::FaceNames face = _aabbFace;
		if (face == voxel::FaceNames::Max) {
			return;
		}
		int uAxis;
		int vAxis;
		ellipseAxes(face, uAxis, vAxis);
		const int wAxis = math::getIndexForAxis(voxel::faceToAxis(face));

		// 4 step offsets in the UV plane (no movement along W)
		glm::ivec3 uvOffsets[4] = {};
		uvOffsets[0][uAxis] = 1;
		uvOffsets[1][uAxis] = -1;
		uvOffsets[2][vAxis] = 1;
		uvOffsets[3][vAxis] = -1;

		// processedAir deduplicates: once an air region is successfully found it is
		// not re-processed if another seed lands inside the same region.
		// It is updated ONLY after a successful bounded BFS so that a failed
		// (unbounded) attempt does not block other axis planes from retrying the
		// same air seed.
		voxelutil::VisitedSet processedAir;

		// BFS air from airSeed locked to the 2D plane defined by searchWAxis at
		// W = airSeed[searchWAxis], stepping in searchUAxis and searchVAxis.
		// Selects adjacent solid voxels if the air region is bounded (enclosed).
		auto tryBfsFromAirSeed = [&](const glm::ivec3 &airSeed, int searchWAxis, int searchUAxis, int searchVAxis) {
			if (!selectionRegion.containsPoint(airSeed)) {
				return;
			}
			if (!voxel::isAir(wrapper.voxel(airSeed).getMaterial())) {
				return;
			}
			// Skip only if a previous successful BFS already covered this air voxel.
			// Using has() (not insert()) so an unbounded attempt on one axis plane
			// does not prevent the seed from being retried on a different axis plane.
			if (processedAir.has(airSeed)) {
				return;
			}

			const int planeW = airSeed[searchWAxis];
			glm::ivec3 searchOffsets[4] = {};
			searchOffsets[0][searchUAxis] = 1;
			searchOffsets[1][searchUAxis] = -1;
			searchOffsets[2][searchVAxis] = 1;
			searchOffsets[3][searchVAxis] = -1;

			core::DynamicArray<glm::ivec3> airRegion;
			airRegion.reserve(64);
			core::DynamicArray<glm::ivec3> bfsQueue;
			bfsQueue.reserve(64);
			voxelutil::VisitedSet visited;
			visited.insert(airSeed);
			bfsQueue.push_back(airSeed);
			bool bounded = true;

			while (!bfsQueue.empty()) {
				const glm::ivec3 current = bfsQueue.back();
				bfsQueue.pop();
				airRegion.push_back(current);

				for (int dir = 0; dir < 4; ++dir) {
					glm::ivec3 neighbor = current + searchOffsets[dir];
					neighbor[searchWAxis] = planeW;

					// Any step outside the volume means the hole is open (unbounded)
					if (!selectionRegion.containsPoint(neighbor)) {
						bounded = false;
						break;
					}
					if (!visited.insert(neighbor)) {
						continue;
					}
					if (!voxel::isAir(wrapper.voxel(neighbor).getMaterial())) {
						continue;
					}
					bfsQueue.push_back(neighbor);
				}
				if (!bounded) {
					break;
				}
			}

			if (!bounded) {
				// Leave processedAir unchanged so other axis planes can retry
				return;
			}

			// Mark the whole bounded air region so sibling seeds skip it
			for (const glm::ivec3 &airPos : airRegion) {
				processedAir.insert(airPos);
			}

			// Select solid voxels in the search plane adjacent to the bounded air region
			for (const glm::ivec3 &airPos : airRegion) {
				for (int dir = 0; dir < 4; ++dir) {
					glm::ivec3 neighbor = airPos + searchOffsets[dir];
					neighbor[searchWAxis] = planeW;
					if (!selectionRegion.containsPoint(neighbor)) {
						continue;
					}
					const voxel::Voxel &vox = wrapper.voxel(neighbor);
					if (!voxel::isAir(vox.getMaterial())) {
						func(neighbor.x, neighbor.y, neighbor.z, vox);
					}
				}
			}
		};

		// Seed type 1: 4 UV neighbors at the same W depth as the clicked voxel.
		// Handles clicking the face that lies flat on the surface containing the
		// hole (e.g. top face for a well opening: the adjacent floor air is
		// directly beside the rim voxel in the same XZ plane).
		for (int startDir = 0; startDir < 4; ++startDir) {
			tryBfsFromAirSeed(_aabbFirstPos + uvOffsets[startDir], wAxis, uAxis, vAxis);
		}

		// Seed type 2: the air voxel one step in the face direction.
		// Handles clicking a side face that directly borders the hole interior
		// (e.g. inner wall of a tube, or rim voxel face pointing into a floor hole).
		// All three axis planes through the air voxel are tried because which plane
		// is correct depends on the geometry, not on the clicked face. The bounded
		// check rejects open planes automatically.
		const int faceSign = voxel::isPositiveFace(face) ? 1 : -1;
		glm::ivec3 faceSeed = _aabbFirstPos;
		faceSeed[wAxis] += faceSign;
		for (int altWAxis = 0; altWAxis < 3; ++altWAxis) {
			const int altUAxis = (altWAxis + 1) % 3;
			const int altVAxis = (altWAxis + 2) % 3;
			tryBfsFromAirSeed(faceSeed, altWAxis, altUAxis, altVAxis);
		}

		break;
	}
	case SelectMode::ColumnRim2D: {
		// Select all solid voxels in a bounded connected protrusion on a face plane.
		//
		// The clicked voxel is the BFS seed. Three axis planes are tried in order,
		// starting with the clicked face's own plane. For each candidate plane, all
		// solid voxels connected to the seed in that 2D slice are collected. If the
		// region reaches the volume boundary it is "unbounded" (a large wall or floor)
		// and that plane is skipped. The first bounded cross-section found is selected.
		//
		// This allows clicking any face of a column:
		//   - Top face: the horizontal cross-section (XZ for a Y-up column) is tried
		//     first and is bounded -> selected immediately.
		//   - Side face: the vertical wall slice is tried first and is typically
		//     unbounded (column touches the floor), so the fallback to the horizontal
		//     cross-section selects the column ring instead.
		//   - Thin-walled pipes work the same way: the wall ring in the cross-section
		//     plane is 4-connected and bounded, so the ring is selected.
		if (voxel::isAir(wrapper.voxel(_aabbFirstPos).getMaterial())) {
			return;
		}

		// Build probe order.
		// If a normal axis is locked, only that plane is tried (no fallback), giving
		// the user explicit control over which cross-section is selected (e.g. always
		// the horizontal XZ circumference of a sphere regardless of clicked face).
		// In Auto mode (Axis::None) the clicked face's plane is tried first, then the other two.
		static constexpr int NumAxes = 3;
		int tryWAxes[NumAxes] = {0, 1, 2};
		int tryCount = NumAxes;
		if (_columnRimNormalAxis != math::Axis::None) {
			tryWAxes[0] = math::getIndexForAxis(_columnRimNormalAxis);
			tryCount = 1;
		} else if (_aabbFace != voxel::FaceNames::Max) {
			const int faceWAxis = math::getIndexForAxis(voxel::faceToAxis(_aabbFace));
			tryWAxes[0] = faceWAxis;
			tryWAxes[1] = (faceWAxis + 1) % NumAxes;
			tryWAxes[2] = (faceWAxis + 2) % NumAxes;
		}

		core::DynamicArray<glm::ivec3> solidRegion;
		solidRegion.reserve(64);
		core::DynamicArray<glm::ivec3> bfsQueue;
		bfsQueue.reserve(64);

		for (int tryIdx = 0; tryIdx < tryCount; ++tryIdx) {
			const int altWAxis = tryWAxes[tryIdx];
			const int altUAxis = (altWAxis + 1) % NumAxes;
			const int altVAxis = (altWAxis + 2) % NumAxes;

			glm::ivec3 searchOffsets[4] = {};
			searchOffsets[0][altUAxis] = 1;
			searchOffsets[1][altUAxis] = -1;
			searchOffsets[2][altVAxis] = 1;
			searchOffsets[3][altVAxis] = -1;

			const int planeW = _aabbFirstPos[altWAxis];

			solidRegion.clear();
			bfsQueue.clear();
			// VisitedSet (DynamicSet) has no clear() so it is created fresh per iteration.
			// At most 3 iterations total and the set is small (bounded cross-sections only).
			voxelutil::VisitedSet visited;
			visited.insert(_aabbFirstPos);
			bfsQueue.push_back(_aabbFirstPos);
			bool bounded = true;

			while (!bfsQueue.empty()) {
				const glm::ivec3 current = bfsQueue.back();
				bfsQueue.pop();
				solidRegion.push_back(current);

				for (int dir = 0; dir < 4; ++dir) {
					glm::ivec3 neighbor = current + searchOffsets[dir];
					neighbor[altWAxis] = planeW;

					// Any step outside the volume means the solid region is open
					if (!selectionRegion.containsPoint(neighbor)) {
						bounded = false;
						break;
					}
					if (!visited.insert(neighbor)) {
						continue;
					}
					if (voxel::isAir(wrapper.voxel(neighbor).getMaterial())) {
						continue;
					}
					bfsQueue.push_back(neighbor);
				}
				if (!bounded) {
					break;
				}
			}

			if (!bounded) {
				continue; // try next axis plane
			}

			// First bounded cross-section found - select it and stop
			for (const glm::ivec3 &solidPos : solidRegion) {
				const voxel::Voxel &vox = wrapper.voxel(solidPos);
				func(solidPos.x, solidPos.y, solidPos.z, vox);
			}
			break;
		}
		break;
	}
	case SelectMode::HoleRim3D: {
		// Find the minimal closed loop of surface voxels that encircles a hole.
		//
		// A "surface voxel" is a solid voxel with at least one 6-face air neighbor.
		// BFS on the surface (26-connected) from the clicked voxel S builds a
		// shortest-path tree. "Non-tree edges" (edges to already-visited nodes that
		// are not the BFS parent) each create a cycle of length dist[u]+1+dist[v].
		// Cycles are tested from shortest to longest. For each candidate, the cycle
		// polygon is projected onto the clicked-face plane and the air-seed voxel A
		// (one step into the hole) is tested for containment using voxelutil::lassoContains().
		// The first cycle that passes is the minimal rim.
		//
		// The projection test is exact for axis-aligned and planar-tilted holes.
		// For strongly curved surfaces the winding-number approximation may fail;
		// in that case the user can fall back to HoleRim2D.
		if (voxel::isAir(wrapper.voxel(_aabbFirstPos).getMaterial())) {
			return;
		}

		// Gate: the clicked voxel must have at least one 6-face air neighbor so
		// that there is a hole for the rim to encircle. Prefer the clicked face
		// direction (the face "looking into" the opening). Fall back to any other
		// 6-face air neighbor so that clicking a face parallel to the hole plane
		// (e.g. +Z face on a ring perpendicular to Z) still works: in that case
		// the face-direction neighbor is the next wall slice (solid).
		const int faceSign = voxel::isPositiveFace(_aabbFace) ? 1 : -1;
		const int faceAxisIdx = math::getIndexForAxis(voxel::faceToAxis(_aabbFace));
		{
			glm::ivec3 preferred = _aabbFirstPos;
			preferred[faceAxisIdx] += faceSign;
			bool found =
				selectionRegion.containsPoint(preferred) && voxel::isAir(wrapper.voxel(preferred).getMaterial());
			if (!found) {
				for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
					const glm::ivec3 nb = _aabbFirstPos + off;
					if (selectionRegion.containsPoint(nb) && voxel::isAir(wrapper.voxel(nb).getMaterial())) {
						found = true;
						break;
					}
				}
			}
			if (!found) {
				return;
			}
		}

		// A surface voxel is solid with at least one 6-face air neighbor
		auto isSurface = [&](const glm::ivec3 &pos) -> bool {
			if (!selectionRegion.containsPoint(pos)) {
				return false;
			}
			if (voxel::isAir(wrapper.voxel(pos).getMaterial())) {
				return false;
			}
			for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
				const glm::ivec3 nb = pos + off;
				if (!selectionRegion.containsPoint(nb)) {
					continue;
				}
				if (voxel::isAir(wrapper.voxel(nb).getMaterial())) {
					return true;
				}
			}
			return false;
		};

		// BFS state - parallel arrays indexed by node ID:
		//   bfsPos[i]    = world position
		//   bfsParent[i] = index of parent node (root: bfsParent[0] = 0)
		//   bfsDist[i]   = shortest-path distance from S (root = node 0)
		//   posToIdx     = pos -> node index for O(1) back-edge lookup
		using PosIndexMap = core::DynamicMap<glm::ivec3, int, 1031, glm::hash<glm::ivec3>>;

		core::DynamicArray<glm::ivec3> bfsPos;
		bfsPos.reserve(HoleRim3DReserve);
		core::DynamicArray<int> bfsParent;
		bfsParent.reserve(HoleRim3DReserve);
		core::DynamicArray<int> bfsDist;
		bfsDist.reserve(HoleRim3DReserve);
		PosIndexMap posToIdx;
		posToIdx.reserve(HoleRim3DReserve);

		bfsPos.push_back(_aabbFirstPos);
		bfsParent.push_back(0); // root's parent points to itself
		bfsDist.push_back(0);
		posToIdx.put(_aabbFirstPos, 0);

		// A non-tree edge (u,v) creates a cycle of length dist[u]+1+dist[v]
		struct NonTreeEdge {
			int uIdx;
			int vIdx;
			int cycleLen;
		};
		core::DynamicArray<NonTreeEdge> nonTreeEdges;
		nonTreeEdges.reserve(HoleRim3DInitialEdgeReserve);

		for (int qi = 0; qi < (int)bfsPos.size(); ++qi) {
			const glm::ivec3 current = bfsPos[qi];
			const int curDist = bfsDist[qi];
			if (curDist >= HoleRim3DMaxSearchRadius) {
				continue;
			}

			// All tree edges and non-tree edges use face-adjacency only.
			// Diagonal non-tree edges create shortcut cycles that skip corner voxels of
			// the rim (e.g. a diagonal from (2,3,z) to (3,2,z) forms a 7-cycle that
			// looks valid but misses the corner voxel at (3,3,z)). Face-only ensures
			// the cycle polygon has no gaps that break voxelutil::lassoContains() or leave rim
			// voxels unselected between consecutive cycle vertices.
			auto tryNeighbor = [&](const glm::ivec3 &nb, bool treeEdge) {
				if (!isSurface(nb)) {
					return;
				}
				const auto it = posToIdx.find(nb);
				if (it == posToIdx.end()) {
					if (!treeEdge) {
						return; // diagonal: don't add new tree nodes
					}
					const int nbIdx = (int)bfsPos.size();
					posToIdx.put(nb, nbIdx);
					bfsPos.push_back(nb);
					bfsParent.push_back(qi);
					bfsDist.push_back(curDist + 1);
				} else {
					// Already visited: potential non-tree edge.
					// Only record once (when nbIdx < qi, nb was visited before current).
					// Skip parent-child edges to avoid trivial 2-cycles.
					const int nbIdx = it->value;
					if (nbIdx >= qi || nbIdx == bfsParent[qi]) {
						return;
					}
					// Skip back-edges: nbIdx is an ancestor of qi in the BFS tree.
					// A back-edge produces a self-intersecting cycle because pathU
					// already contains nbIdx, so the polygon visits it twice (L-shape).
					// Walk distDiff steps up from qi; if we land on nbIdx, skip.
					const int distDiff = curDist - bfsDist[nbIdx];
					int walkIdx = qi;
					for (int step = 0; step < distDiff; ++step) {
						walkIdx = bfsParent[walkIdx];
					}
					if (walkIdx == nbIdx) {
						return; // back-edge: skip
					}
					const int cycleLen = curDist + 1 + bfsDist[nbIdx];
					if (cycleLen >= HoleRim3DMinCycleLen) {
						nonTreeEdges.push_back({qi, nbIdx, cycleLen});
					}
				}
			};

			for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
				tryNeighbor(current + off, true);
			}
		}

		if (nonTreeEdges.empty()) {
			return;
		}

		// Trace BFS tree path from nodeIdx back to root S (inclusive)
		// outPath = [nodePos, ..., S]
		auto tracePath = [&](int nodeIdx, core::DynamicArray<glm::ivec3> &outPath) {
			outPath.clear();
			outPath.reserve(HoleRim3DMaxSearchRadius);
			int idx = nodeIdx;
			while (true) {
				outPath.push_back(bfsPos[idx]);
				if (idx == 0) {
					break;
				}
				idx = bfsParent[idx];
			}
		};

		// Process non-tree edges from shortest cycle to longest.
		// For each candidate: build the cycle polygon and test whether its
		// centroid lies inside the projected polygon. The centroid of the rim
		// polygon is at the hole center, strictly inside for any convex/typical hole.
		// The 3D centroid is also checked to be an air voxel to confirm it is inside
		// the hole, not on the solid surface.
		// Back-edges were already filtered above, so every cycle is a simple polygon.
		// Projection axes are determined per-cycle from planarity (see below).
		core::DynamicArray<glm::ivec3> pathU;
		core::DynamicArray<glm::ivec3> pathV;
		core::DynamicArray<glm::ivec3> cycle;
		cycle.reserve(HoleRim3DMaxSearchRadius * 2);

		while (!nonTreeEdges.empty()) {
			// Selection-extract minimum cycle-length edge
			int minIdx = 0;
			for (int i = 1; i < (int)nonTreeEdges.size(); ++i) {
				if (nonTreeEdges[i].cycleLen < nonTreeEdges[minIdx].cycleLen) {
					minIdx = i;
				}
			}
			const NonTreeEdge edge = nonTreeEdges[minIdx];
			// Swap-and-pop removal
			nonTreeEdges[minIdx] = nonTreeEdges.back();
			nonTreeEdges.resize(nonTreeEdges.size() - 1);

			// Build cycle: [S,...,u] + [v,...,last_before_S]
			// pathU = [u,...,S], pathV = [v,...,S]
			tracePath(edge.uIdx, pathU);
			tracePath(edge.vIdx, pathV);

			cycle.clear();
			// Forward segment: reverse pathU to get [S,...,u]
			for (int i = (int)pathU.size() - 1; i >= 0; --i) {
				cycle.push_back(pathU[i]);
			}
			// Back segment: [v,...,last_before_S] (exclude final S to avoid duplicate)
			for (int i = 0; i < (int)pathV.size() - 1; ++i) {
				cycle.push_back(pathV[i]);
			}

			// Reject figure-8 cycles: pathU and pathV may share an ancestor other than S
			// (their LCA). The concatenation then revisits those nodes -> self-intersecting
			// there-and-back path. Require every position in the cycle to be unique.
			{
				voxelutil::VisitedSet cycleSet;
				bool hasDuplicate = false;
				for (const glm::ivec3 &cp : cycle) {
					if (cycleSet.has(cp)) {
						hasDuplicate = true;
						break;
					}
					cycleSet.insert(cp);
				}
				if (hasDuplicate) {
					continue;
				}
			}

			// Determine projection plane from cycle planarity.
			// For a planar ring (all voxels at the same value of one axis) the correct
			// 2D projection is onto the plane perpendicular to that constant axis.
			// Using the clicked face's projection can be wrong when the airSeed was
			// found via fallback in a different direction (e.g. +Z face on an XZ-plane
			// floor: the ring is in XZ, not XY). Detect the minimum-spread axis of the
			// cycle and project onto the other two. Fall back to the face axis if no
			// single axis has strictly smaller spread (non-planar ring).
			int cycleUAxis;
			int cycleVAxis;
			int cycleFaceAxisIdx;
			{
				glm::ivec3 minC = cycle[0];
				glm::ivec3 maxC = cycle[0];
				for (const glm::ivec3 &cv : cycle) {
					minC = glm::min(minC, cv);
					maxC = glm::max(maxC, cv);
				}
				const glm::ivec3 spread = maxC - minC;
				cycleFaceAxisIdx = faceAxisIdx; // default: use clicked-face axis
				for (int ax = 0; ax < 3; ++ax) {
					if (spread[ax] < spread[cycleFaceAxisIdx]) {
						cycleFaceAxisIdx = ax;
					}
				}
				cycleUAxis = (cycleFaceAxisIdx + 1) % 3;
				cycleVAxis = (cycleFaceAxisIdx + 2) % 3;
			}

			// Test: is the cycle a closed rim around the hole?
			// airSeed always projects to S's position (a polygon vertex) -> unreliable
			// boundary case in ray casting. Use the cycle centroid instead: for a rim
			// polygon the centroid is at the hole center, which is strictly inside.
			int centU = 0;
			int centV = 0;
			int centDepth = 0;
			for (const glm::ivec3 &cv : cycle) {
				centU += cv[cycleUAxis];
				centV += cv[cycleVAxis];
				centDepth += cv[cycleFaceAxisIdx];
			}
			centU = (int)glm::round((double)centU / (double)cycle.size());
			centV = (int)glm::round((double)centV / (double)cycle.size());
			centDepth = (int)glm::round((double)centDepth / (double)cycle.size());
			if (!voxelutil::lassoContains(cycle, centU, centV, cycleUAxis, cycleVAxis)) {
				continue;
			}

			// Hole check: the 3D centroid of the cycle must be an air voxel (hole
			// interior). A rim polygon's centroid is at the hole center (air); a
			// wall-surface or floor cycle centroid lands on solid.
			// Use the cycle's own average depth along cycleFaceAxisIdx, not airSeed's
			// depth: airSeed may be on the opposite side of the surface from the hole
			// (e.g. +Y click on a floor hole puts airSeed at Y+1 = always air, which
			// would let every floor cycle pass the check erroneously).
			glm::ivec3 centroidTest;
			centroidTest[cycleUAxis] = centU;
			centroidTest[cycleVAxis] = centV;
			centroidTest[cycleFaceAxisIdx] = centDepth;
			if (!selectionRegion.containsPoint(centroidTest)) {
				continue;
			}
			if (!voxel::isAir(wrapper.voxel(centroidTest).getMaterial())) {
				continue;
			}

			// Valid rim found: select all voxels in the cycle
			for (const glm::ivec3 &pos : cycle) {
				const voxel::Voxel &vox = wrapper.voxel(pos);
				if (!voxel::isAir(vox.getMaterial())) {
					func(pos.x, pos.y, pos.z, vox);
				}
			}
			break;
		}
		break;
	}
	case SelectMode::Paint: {
		const glm::ivec3 center = ctx.cursorPosition;
		const int rad = radius();
		const int radSq = rad * rad;
		const bool growOnly = _paintGrowRegion && wrapper.modifierType() != ModifierType::Erase;
		voxelutil::VisitSolid condition;
		auto paintFunc = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			const int dx = x - center.x;
			const int dy = y - center.y;
			const int dz = z - center.z;
			if (dx * dx + dy * dy + dz * dz > radSq) {
				return;
			}
			if (growOnly) {
				bool hasSelectedNeighbor = false;
				for (const glm::ivec3 &off : ::voxel::arrayPathfinderFaces) {
					const glm::ivec3 npos(x + off.x, y + off.y, z + off.z);
					if (ctx.targetVolumeRegion.containsPoint(npos)) {
						const ::voxel::Voxel &neighborVoxel = wrapper.voxel(npos);
						if (!::voxel::isAir(neighborVoxel.getMaterial()) &&
							(neighborVoxel.getFlags() & ::voxel::FlagOutline)) {
							hasSelectedNeighbor = true;
							break;
						}
					}
				}
				if (!hasSelectedNeighbor) {
					return;
				}
			}
			func(x, y, z, voxel);
		};
		voxelutil::visitVolume(wrapper, selectionRegion, paintFunc, condition);
		_paintDirtyRegion.accumulate(selectionRegion);
		break;
	}
	case SelectMode::Max:
		return;
	}
}

void SelectBrush::redrawEdgesOnVolume(voxel::RawVolume *volume, const voxel::Region &region, voxel::Region &outDirty) {
	const int vertexCount = (int)_lassoPath.size();
	if (vertexCount < 2) {
		return;
	}
	const int uAxis = _lassoUAxis;
	const int vAxis = _lassoVAxis;
	const int wAxis = _lassoFaceAxisIdx;
	const bool positiveNormal = voxel::isPositiveFace(_lassoFace);

	auto edgeFunc = [&](int x, int y, int z, const voxel::Voxel &v) {
		const glm::ivec3 pos(x, y, z);
		if (!_lassoEdgeHistory.hasVoxel(pos)) {
			_lassoEdgeHistory.setVoxel(x, y, z, v);
		}
		voxel::Voxel flagged = v;
		flagged.setFlags(flagged.getFlags() | voxel::FlagOutline);
		volume->setVoxel(pos, flagged);
		outDirty.accumulate(pos);
	};
	auto readVoxel = [&](const glm::ivec3 &pos) { return volume->voxel(pos); };
	for (int edgeIdx = 1; edgeIdx < vertexCount; ++edgeIdx) {
		voxelutil::drawLassoEdgeSurface(readVoxel, _lassoPath[edgeIdx - 1], _lassoPath[edgeIdx], uAxis, vAxis, wAxis,
										positiveNormal, region, edgeFunc);
	}
}

void SelectBrush::invalidateLasso() {
	_lassoAccumulating = false;
	_lassoPath.clear();
	_lassoEdgeHistory.clear();
}

void SelectBrush::popLastVertex() {
	if (!_lassoPath.empty()) {
		_lassoPath.resize(_lassoPath.size() - 1);
	}
}

void SelectBrush::invalidateEllipse() {
	_ellipseValid = false;
	_ellipseHistory.clear();
}

void SelectBrush::invalidateSlope() {
	_slopeValid = false;
	_slopeHistory.clear();
}

} // namespace voxedit
