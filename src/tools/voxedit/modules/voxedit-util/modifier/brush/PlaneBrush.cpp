/**
 * @file
 */

#include "PlaneBrush.h"
#include "math/Axis.h"
#include "math/Ray.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

bool PlaneBrush::beginBrush(const BrushContext &ctx) {
	if (!Super::beginBrush(ctx)) {
		return false;
	}
	_hitVoxel = ctx.hitCursorVoxel;
	_initialPlanePos = ctx.cursorPosition;
	return true;
}

int PlaneBrush::calculateThickness(const BrushContext &ctx) const {
	const math::Axis axis = voxel::faceToAxis(_boxGesture.face());
	const int idx = math::getIndexForAxis(axis);
	const voxel::Region region = Super::calcRegion(ctx);
	if (ctx.modifierType == ModifierType::Place) {
		// don't allow negative direction - only allow growth into the face direction
		if (voxel::isNegativeFace(_boxGesture.face())) {
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

bool PlaneBrush::projectCursor(const math::Ray &ray, glm::ivec3 &position) const {
	const math::Axis axis = voxel::faceToAxis(_boxGesture.face());
	if (!_boxGesture.active() || axis == math::Axis::None) {
		return false;
	}
	position = _initialPlanePos;
	const int idx = math::getIndexForAxis(axis);
	const glm::vec3 direction = glm::normalize(ray.direction);
	const float denominator = 1.0f - direction[idx] * direction[idx];
	if (denominator > 0.0001f) {
		// Closest point on the extrusion axis to the mouse ray. Picking the
		// source surface again would keep the height fixed throughout the drag.
		const glm::vec3 anchor = glm::vec3(_initialPlanePos) + 0.5f;
		const glm::vec3 offset = ray.origin - anchor;
		const float distance = (offset[idx] - direction[idx] * glm::dot(offset, direction)) / denominator;
		position[idx] = (int)glm::floor(anchor[idx] + distance);
	}
	return true;
}

voxel::Region PlaneBrush::previewRegion(const BrushContext &ctx, const voxel::RawVolume *volume) const {
	if (volume == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	PlaneBrush prepared(*this);
	prepared.preExecute(ctx, volume);
	return prepared.calcRegion(ctx);
}

void PlaneBrush::preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) {
	Super::preExecute(ctx, volume);
	if (!_boxGesture.active()) {
		_boxGesture.clearSecondPosition();
		return;
	}
	// we only need to calculate the second position once
	if (_boxGesture.hasSecondPosition()) {
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
		region = voxelutil::extrudePlaneRegion(*volume, _initialPlanePos, _boxGesture.face(), _hitVoxel, ctx.cursorVoxel, 1);
	} else if (ctx.modifierType == ModifierType::Erase) {
		region = voxelutil::erasePlaneRegion(*volume, _initialPlanePos, _boxGesture.face(), _hitVoxel);
	} else if (ctx.modifierType == ModifierType::Override) {
		region = voxelutil::overridePlaneRegion(*volume, _initialPlanePos, _boxGesture.face(), ctx.cursorVoxel);
	}
	// after we have calculated the region we use the mins and maxs and only use the mouse
	// cursor position in the volume to determine the 3rd dimension of the brush aabb.
	if (region.isValid()) {
		_boxGesture.setPlaneBounds(region);
	}
}

void PlaneBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region) {
	if (_boxGesture.face() == voxel::FaceNames::Max) {
		_boxGesture.setFace(ctx.cursorFace);
	}
	if (_boxGesture.face() == voxel::FaceNames::Max) {
		return;
	}
	const int thickness = calculateThickness(ctx);
	if (thickness <= 0) {
		return;
	}
	if (ctx.modifierType == ModifierType::Place) {
		voxelutil::extrudePlane(wrapper, _initialPlanePos, _boxGesture.face(), _hitVoxel, ctx.cursorVoxel, thickness);
	} else if (ctx.modifierType == ModifierType::Erase) {
		voxelutil::erasePlane(wrapper, _initialPlanePos, _boxGesture.face(), _hitVoxel, thickness);
	} else if (ctx.modifierType == ModifierType::Override) {
		voxelutil::overridePlane(wrapper, _initialPlanePos, _boxGesture.face(), _hitVoxel, thickness);
	}
}

} // namespace voxedit
