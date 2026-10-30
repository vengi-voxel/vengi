/**
 * @file
 */

#include "ExtrudeBrush.h"
#include "app/App.h"
#include "app/ForParallel.h"
#include "core/Common.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Connectivity.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/gtc/matrix_transform.hpp>

namespace voxedit {

ExtrudeBrush::ExtrudeBrush(const ExtrudeBrush &other) : Super(other), ExtrudeOperation() {
	setParameters(other.parameters());
}

void ExtrudeBrush::onSceneChange() {
	Super::onSceneChange();
	_active = false;
	_history.clear();
	_cachedSelBBoxValid = false;
}

void ExtrudeBrush::onActivated() {
	clearOperation();
}

void ExtrudeBrush::reset() {
	Super::reset();
	_parameters.depth = 0;
	_parameters.offsetU = 0;
	_parameters.offsetV = 0;
	_parameters.face = voxel::FaceNames::Max;
	_active = false;
	_history.clear();
	_cachedSelectedPositions.clear();
	_cachedWallCandidates.clear();
	_cachedSelectedBBox = voxel::Region::InvalidRegion;
	_hasCachedSelection = false;
	_capturedVolumeLower = glm::ivec3(0);
	_cachedSelBBoxValid = false;
	_parameters.fillWalls = true;
}

bool ExtrudeBrush::beginBrush(const BrushContext &ctx) {
	if (_active) {
		return false;
	}
	// Only update the stored face when the cursor is actually over a voxel face.
	// When triggered from the panel (cursorFace == Max), keep the last face from a viewport click.
	if (ctx.cursorFace != voxel::FaceNames::Max && !ctx.brushGizmoActive) {
		_parameters.face = ctx.cursorFace;
	}
	_active = true;
	return true;
}

void ExtrudeBrush::endBrush(BrushContext &) {
	_active = false;
}

void ExtrudeBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	prepare(volume, ctx.targetVolumeRegion);
}

bool ExtrudeBrush::active() const {
	return _active || _cachedSelBBoxValid;
}

voxel::Region ExtrudeBrush::calcRegion(const BrushContext &ctx) const {
	if (!_cachedSelBBoxValid || _parameters.face == voxel::FaceNames::Max) {
		return ctx.targetVolumeRegion;
	}
	// Expand the selection bbox along the extrusion direction to cover the
	// extruded/carved voxels, plus lateral offsets and a small side-wall margin
	// on perpendicular axes. Only expand in the direction that is actually affected
	// to keep the region tight (large regions exceed maxPreviewRegion).
	const math::Axis axis = voxel::faceToAxis(_parameters.face);
	const int axisIdx = math::getIndexForAxis(axis);
	const int faceSign = voxel::isNegativeFace(_parameters.face) ? -1 : 1;
	const bool carving = _parameters.depth < 0;
	const int dirSign = carving ? -faceSign : faceSign;
	const int steps = glm::abs(_parameters.depth);

	static constexpr int NumAxes = 3;
	const int perp1 = (axisIdx + 1) % NumAxes;
	const int perp2 = (axisIdx + 2) % NumAxes;

	glm::ivec3 lo = _cachedSelBBox.getLowerCorner();
	glm::ivec3 hi = _cachedSelBBox.getUpperCorner();

	// Extend along extrusion axis by depth steps
	if (dirSign > 0) {
		hi[axisIdx] += steps;
	} else {
		lo[axisIdx] -= steps;
	}

	// Lateral offset shifts
	if (_parameters.offsetU > 0) {
		hi[perp1] += _parameters.offsetU;
	} else if (_parameters.offsetU < 0) {
		lo[perp1] += _parameters.offsetU;
	}
	if (_parameters.offsetV > 0) {
		hi[perp2] += _parameters.offsetV;
	} else if (_parameters.offsetV < 0) {
		lo[perp2] += _parameters.offsetV;
	}

	// Small margin for side walls on perpendicular axes
	static constexpr int SideWallMargin = 1;
	lo[perp1] -= SideWallMargin;
	hi[perp1] += SideWallMargin;
	lo[perp2] -= SideWallMargin;
	hi[perp2] += SideWallMargin;

	voxel::Region region(lo, hi);
	region.cropTo(ctx.targetVolumeRegion);
	return region;
}

void ExtrudeBrush::setDepth(int depth) {
	markSettingsChanged();
	if (_parameters.depth == depth) {
		return;
	}
	_parameters.depth = depth;
	markDirty();
}

void ExtrudeBrush::setOffsetU(int offset) {
	markSettingsChanged();
	if (_parameters.offsetU == offset) {
		return;
	}
	_parameters.offsetU = offset;
	markDirty();
}

void ExtrudeBrush::setOffsetV(int offset) {
	markSettingsChanged();
	if (_parameters.offsetV == offset) {
		return;
	}
	_parameters.offsetV = offset;
	markDirty();
}

void ExtrudeBrush::generate(scenegraph::SceneGraph &, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
							const voxel::Region &) {
	_parameters.voxel = ctx.cursorVoxel;
	evaluate(wrapper);
	}

void ExtrudeBrush::handles(const BrushContext &ctx, core::DynamicArray<BrushGizmoState> &output) const {
	if (hasHandle(ctx)) {
		BrushGizmoState handle;
		describeHandle(ctx, handle);
		output.push_back(handle);
	}
}

bool ExtrudeBrush::hasHandle(const BrushContext &ctx) const {
	return _cachedSelBBoxValid && _parameters.face != voxel::FaceNames::Max;
}

void ExtrudeBrush::describeHandle(const BrushContext &ctx, BrushGizmoState &state) const {
	if (!_cachedSelBBoxValid || _parameters.face == voxel::FaceNames::Max) {
		state.operations = BrushGizmo_None;
		return;
	}

	const glm::vec3 center = glm::vec3(_cachedSelBBox.getCenter());
	const glm::vec3 normal = voxel::faceNormal(_parameters.face);

	// Position the gizmo at the selection center, offset by current depth along the face normal
	state.matrix = glm::translate(glm::mat4(1.0f), center + normal * (float)_parameters.depth);

	// Only allow translation along the extrusion axis
	const math::Axis axis = voxel::faceToAxis(_parameters.face);
	switch (axis) {
	case math::Axis::X:
		state.operations = BrushGizmo_TranslateX;
		break;
	case math::Axis::Y:
		state.operations = BrushGizmo_TranslateY;
		break;
	case math::Axis::Z:
		state.operations = BrushGizmo_TranslateZ;
		break;
	default:
		state.operations = BrushGizmo_None;
		return;
	}

	state.snap[0] = state.snap[1] = state.snap[2] = 1.0f;
	state.localMode = false;
}

bool ExtrudeBrush::editHandle(uint32_t handleId, BrushContext &ctx, const glm::mat4 &matrix,
								   const glm::mat4 &deltaMatrix, uint32_t operation) {
	if (_parameters.face == voxel::FaceNames::Max) {
		return false;
	}

	const math::Axis axis = voxel::faceToAxis(_parameters.face);
	const int axisIdx = math::getIndexForAxis(axis);
	const glm::vec3 translation(deltaMatrix[3]);
	const int delta = (int)glm::round(translation[axisIdx]);
	if (delta == 0) {
		return false;
	}

	// Convert the axis translation to a signed depth change
	const int faceSign = voxel::isNegativeFace(_parameters.face) ? -1 : 1;
	setDepth(_parameters.depth + delta * faceSign);
	return true;
}

} // namespace voxedit
