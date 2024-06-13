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

int PlaneBrush::calculateThickness(const BrushContext &context, ModifierType type) const {
	const math::Axis axis = voxel::faceToAxis(_aabbFace);
	const int idx = math::getIndexForAxis(axis);
	const voxel::Region region = Super::calcRegion(context);
	if (type == ModifierType::Place) {
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

template<class Volume>
static int generateForModifier(Volume &volume, ModifierType type, const BrushContext &context, const glm::ivec3 &pos,
								voxel::FaceNames face, const voxel::Voxel &initialPosVoxel, int thickness) {
	if (face == voxel::FaceNames::Max) {
		face = context.cursorFace;
	}
	int n = 0;
	if (type == ModifierType::Place) {
		n = voxelutil::extrudePlane(volume, pos, face, initialPosVoxel, context.cursorVoxel, thickness);
	} else if (type == ModifierType::Erase) {
		// TODO: support erasing more than one voxel - support thickness
		n = voxelutil::erasePlane(volume, pos, face, initialPosVoxel);
	} else if (type == ModifierType::Override) {
		// TODO: support overriding more than one voxel - support thickness
		n = voxelutil::overridePlane(volume, pos, face, initialPosVoxel);
	}
	return n;
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

	// TODO: allow to specify the modifier type
	ModifierType type = ModifierType::Place;

	voxel::Region region;
	if (type == ModifierType::Place) {
		region =
			voxelutil::extrudePlaneRegion(*volume, _initialPlanePos, _aabbFace, _hitVoxel, context.cursorVoxel, 1);
	} else if (type == ModifierType::Erase) {
		region = voxelutil::erasePlaneRegion(*volume, _initialPlanePos, _aabbFace, _hitVoxel);
	} else if (type == ModifierType::Override) {
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
	const int thickness = calculateThickness(context, wrapper.modifierType());
	if (thickness <= 0) {
		return;
	}
	generateForModifier(wrapper, wrapper.modifierType(), context, _initialPlanePos, _aabbFace, _hitVoxel, thickness);
}

} // namespace voxedit
