/**
 * @file
 */

#include "SelectBrush.h"
#include "LUASelectionMode.h"
#include "voxedit-util/SceneManager.h"
#include "core/collection/DynamicMap.h"
#include "scenegraph/SceneGraph.h"
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
	_paintHadSelection = false;
	_paintDirtyRegion = voxel::Region::InvalidRegion;
	_sceneModifiedFlags = SceneModifiedFlags::All;
	_paintFinalUndoRegion = voxel::Region::InvalidRegion;
	_box3DSelectionRegion = voxel::Region::InvalidRegion;
}

void SelectBrush::onSceneChange() {
	Super::onSceneChange();
	_lassoPath.clear();
	_lassoEdgeHistory.clear();
	_lassoAccumulating = false;
	_paintAccumulating = false;
	_paintHadSelection = false;
	_paintDirtyRegion = voxel::Region::InvalidRegion;
	_sceneModifiedFlags = SceneModifiedFlags::All;
	_box3DSelectionRegion = voxel::Region::InvalidRegion;
}

void SelectBrush::abort(BrushContext &ctx) {
	if (_selectMode == SelectMode::Lasso && _lassoAccumulating) {
		_lassoPath.clear();
		_lassoAccumulating = false;
		_sceneModifiedFlags = SceneModifiedFlags::All;
	}
	if (_selectMode == SelectMode::Paint && _paintAccumulating) {
		_paintAccumulating = false;
		_paintHadSelection = false;
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

void SelectBrush::setLuaSelectionMode(int index, LUASelectionMode *mode) {
	_luaSelectionModeIndex = index;
	_activeLuaSelectionMode = mode;
	if (index >= 0 && mode != nullptr) {
		_selectMode = SelectMode::Script;
	} else if (_selectMode == SelectMode::Script) {
		_selectMode = SelectMode::All;
	}
}

bool SelectBrush::needsAdditionalAction(const BrushContext &ctx) const {
	// Circle, Connected, SameColor, Surface, FuzzyColor, FlatSurface use their own
	// selection logic -they don't benefit from the 3-click AABB workflow.
	// Only All and Box3D use the standard AABB region.
	if (_selectMode != SelectMode::All && _selectMode != SelectMode::Box3D) {
		return false;
	}
	// TODO: this must be forwarded to the lua selection mode implementations instead of hardcoded here.
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
		_paintHadSelection = false;
		_paintDirtyRegion = voxel::Region::InvalidRegion;
		_paintFinalUndoRegion = voxel::Region::InvalidRegion;
		_sceneModifiedFlags = SceneModifiedFlags::NoUndo;
	}
	return Super::beginBrush(ctx);
}

voxel::Region SelectBrush::calcRegion(const BrushContext &ctx) const {
	if (_selectMode == SelectMode::Script) {
		return ctx.targetVolumeRegion;
	}
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
	// Delegate to lua selection mode if active
	if (_selectMode == SelectMode::Script && _activeLuaSelectionMode != nullptr) {
		_activeLuaSelectionMode->execute(sceneGraph, wrapper, ctx, region, _aabbFirstPos, _aabbFace);
		if (_sceneManager) {
			const voxelgenerator::LuaDirtyRegions &dirtyRegions = _activeLuaSelectionMode->dirtyRegions();
			Log::debug("SelectBrush::generate: %i dirty regions after lua execution", (int)dirtyRegions.size());
			for (const auto &entry : dirtyRegions) {
				const int dirtyNodeId = entry->key;
				const voxel::Region &dirtyRegion = entry->value;
				if (dirtyRegion.isValid()) {
					Log::debug("SelectBrush::generate: forwarding dirty region for node %i: %s",
							   dirtyNodeId, dirtyRegion.toString().c_str());
					_sceneManager->modified(dirtyNodeId, dirtyRegion);
				}
			}
		}
		return;
	}

	voxel::Region selectionRegion = region;
	if (_brushClamping) {
		selectionRegion.cropTo(ctx.targetVolumeRegion);
	}

	// Clear box region by default; Box3D case sets it below
	_box3DSelectionRegion = voxel::Region::InvalidRegion;

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
	case SelectMode::Box3D: {
		voxelutil::VisitSolid condition;
		voxelutil::visitVolumeParallel(wrapper, selectionRegion, func, condition);
		// Store the exact box region so ModifierVolumeWrapper::skip() allows
		// editing any position inside the box (including air voxels)
		if (wrapper.modifierType() == ModifierType::Erase) {
			_box3DSelectionRegion = voxel::Region::InvalidRegion;
		} else {
			_box3DSelectionRegion = selectionRegion;
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
	case SelectMode::Paint: {
		const glm::ivec3 center = ctx.cursorPosition;
		const int rad = radius();
		const int radSq = rad * rad;
		if (!_paintDirtyRegion.isValid() && !_paintHadSelection) {
			const int activeNodeId = sceneGraph.activeNode();
			if (sceneGraph.hasNode(activeNodeId)) {
				_paintHadSelection = sceneGraph.node(activeNodeId).hasSelection();
			}
		}
		const bool growOnly = _paintGrowRegion && wrapper.modifierType() != ModifierType::Erase
			&& (_paintHadSelection || _paintDirtyRegion.isValid());
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
	case SelectMode::Script:
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

void SelectBrush::popLastLassoPathEntry() {
	if (!_lassoPath.empty()) {
		_lassoPath.pop();
	}
}

void SelectBrush::invalidateEllipse() {
	_ellipseValid = false;
	_ellipseHistory.clear();
}

} // namespace voxedit
