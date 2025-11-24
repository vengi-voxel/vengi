/**
 * @file
 */

#include "PlaneBrush.h"
#include "core/Log.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

bool PlaneBrush::start(const BrushContext &ctx) {
	if (!Super::start(ctx)) {
		return false;
	}
	_hitVoxel = ctx.hitCursorVoxel;
	_initialPlanePos = ctx.cursorPosition;
	return true;
}

int PlaneBrush::calculateThickness(const BrushContext &ctx) const {
	const math::Axis axis = voxel::faceToAxis(_aabbFace);
	const int idx = math::getIndexForAxis(axis);
	const voxel::Region region = Super::calcRegion(ctx);
	if (ctx.modifierType == ModifierType::Place) {
		// don't allow negative direction - only allow growth into the face direction
		if (voxel::isNegativeFace(_aabbFace)) {
			if (_initialPlanePos[idx] < ctx.cursorPosition[idx]) {
				return 0;
			}
		} else {
			if (_initialPlanePos[idx] > ctx.cursorPosition[idx]) {
				return 0;
			}
		}
	}
	return core_max(1, region.getDimensionsInVoxels()[idx]);
}

void PlaneBrush::reset() {
	Super::reset();
	_hitVoxel = voxel::Voxel();
	_initialPlanePos = glm::ivec3(0);
}

void PlaneBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	Super::preExecute(ctx, volume);
	if (!_aabbMode) {
		_secondPosValid = false;
		return;
	}
	// we only need to calculate the second position once
	if (_secondPosValid) {
		return;
	}
	// here we determine the surface region of the extrusion plane
	// and define the mins and maxs of the region to span the aabb
	// for the brush.
	//
	// for the plane algorithm we still have to use the position
	// where the cursor was first pressed because the underlying
	// voxel might be different for the region mins or maxs - or
	// might even be air

	voxel::Region region;
	if (ctx.modifierType == ModifierType::Place) {
		region = voxelutil::extrudePlaneRegion(*volume, _initialPlanePos, _aabbFace, _hitVoxel, ctx.cursorVoxel, 1);
	} else if (ctx.modifierType == ModifierType::Erase) {
		region = voxelutil::erasePlaneRegion(*volume, _initialPlanePos, _aabbFace, _hitVoxel);
	} else if (ctx.modifierType == ModifierType::Override) {
		region = voxelutil::overridePlaneRegion(*volume, _initialPlanePos, _aabbFace, ctx.cursorVoxel);
	}
	_aabbFirstPos = region.getLowerCorner();
	_aabbSecondPos = region.getUpperCorner();
	// after we have calculated the region we use the mins and maxs and only use the mouse
	// cursor position in the volume to determine the 3rd dimension of the brush aabb.
	_secondPosValid = true;
}

void PlaneBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						  const BrushContext &ctx, const voxel::Region &region) {
	const int thickness = calculateThickness(ctx);
	if (thickness <= 0) {
		return;
	}
	if (_aabbFace == voxel::FaceNames::Max) {
		_aabbFace = ctx.cursorFace;
	}
	if (ctx.modifierType == ModifierType::Place) {
		voxelutil::extrudePlane(wrapper, _initialPlanePos, _aabbFace, _hitVoxel, ctx.cursorVoxel, thickness);
	} else if (ctx.modifierType == ModifierType::Erase) {
		// TODO: BRUSH: support erasing more than one voxel - support thickness
		voxelutil::erasePlane(wrapper, _initialPlanePos, _aabbFace, _hitVoxel);
	} else if (ctx.modifierType == ModifierType::Override) {
		// TODO: BRUSH: support overriding more than one voxel - support thickness
		voxelutil::overridePlane(wrapper, _initialPlanePos, _aabbFace, _hitVoxel);
	}
}

} // namespace voxedit
