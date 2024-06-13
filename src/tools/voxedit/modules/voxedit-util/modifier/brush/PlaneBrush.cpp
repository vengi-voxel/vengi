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

bool PlaneBrush::start(const BrushContext &context) {
	if (!Super::start(context)) {
		return false;
	}
	_hitVoxel = context.hitCursorVoxel;
	_initialPlanePos = context.cursorPosition;
	return true;
}

int PlaneBrush::calculateThickness(const BrushContext &context) const {
	const math::Axis axis = voxel::faceToAxis(_aabbFace);
	const int idx = math::getIndexForAxis(axis);
	const voxel::Region region = Super::calcRegion(context);
	if (context.modifierType == ModifierType::Place) {
		// don't allow negative direction - only allow growth into the face direction
		if (voxel::isNegativeFace(_aabbFace)) {
			if (_initialPlanePos[idx] < context.cursorPosition[idx]) {
				return 0;
			}
		} else {
			if (_initialPlanePos[idx] > context.cursorPosition[idx]) {
				return 0;
			}
		}
		// extrude is taking the voxel at the cursor position into account and thus we get
		// a region of the thickness of 2 - we have to reduce this again to make the aabb
		// brush span the 3rd dimension correctly.
		return core_max(1, region.getDimensionsInVoxels()[idx] - 1);
	}
	return core_max(1, region.getDimensionsInVoxels()[idx]);
}

void PlaneBrush::reset() {
	Super::reset();
	_hitVoxel = voxel::Voxel();
	_initialPlanePos = glm::ivec3(0);
}

void PlaneBrush::preExecute(const BrushContext &context, const voxel::RawVolume *volume) {
	Super::preExecute(context, volume);
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
	if (context.modifierType == ModifierType::Place) {
		region = voxelutil::extrudePlaneRegion(*volume, _initialPlanePos, _aabbFace, _hitVoxel, context.cursorVoxel, 1);
	} else if (context.modifierType == ModifierType::Erase) {
		region = voxelutil::erasePlaneRegion(*volume, _initialPlanePos, _aabbFace, _hitVoxel);
	} else if (context.modifierType == ModifierType::Override) {
		region = voxelutil::overridePlaneRegion(*volume, _initialPlanePos, _aabbFace, context.cursorVoxel);
	}
	_aabbFirstPos = region.getLowerCorner();
	_aabbSecondPos = region.getUpperCorner();
	// after we have calculated the region we use the mins and maxs and only use the mouse
	// cursor position in the volume to determine the 3rd dimension of the brush aabb.
	_secondPosValid = true;
}

void PlaneBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						  const BrushContext &context, const voxel::Region &region) {
	const int thickness = calculateThickness(context);
	if (thickness <= 0) {
		return;
	}
	if (_aabbFace == voxel::FaceNames::Max) {
		_aabbFace = context.cursorFace;
	}
	if (context.modifierType == ModifierType::Place) {
		voxelutil::extrudePlane(wrapper, _initialPlanePos, _aabbFace, _hitVoxel, context.cursorVoxel, thickness);
	} else if (context.modifierType == ModifierType::Erase) {
		// TODO: support erasing more than one voxel - support thickness
		voxelutil::erasePlane(wrapper, _initialPlanePos, _aabbFace, _hitVoxel);
	} else if (context.modifierType == ModifierType::Override) {
		// TODO: support overriding more than one voxel - support thickness
		voxelutil::overridePlane(wrapper, _initialPlanePos, _aabbFace, _hitVoxel);
	}
}

} // namespace voxedit
