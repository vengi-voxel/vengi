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
	return true;
}

voxel::Region PlaneBrush::calcRegion(const BrushContext &context) const {
	return _region;
}

int PlaneBrush::calculateThickness(const BrushContext &context) const {
	const math::Axis axis = voxel::faceToAxis(_aabbFace);
	const int idx = math::getIndexForAxis(axis);
	const voxel::Region region = Super::calcRegion(context);
	return core_max(1, region.getDimensionsInVoxels()[idx]);
}

template<class Volume>
static void generateForModifier(Volume &volume, ModifierType type, const BrushContext &context, const glm::ivec3 &pos,
								voxel::FaceNames face, const voxel::Voxel &initialPosVoxel, int thickness) {
	if (type == ModifierType::Place) {
		voxelutil::extrudePlane(volume, pos, face, initialPosVoxel, context.cursorVoxel, thickness);
	} else if (type == ModifierType::Erase) {
		voxelutil::erasePlane(volume, pos, face, initialPosVoxel);
	} else if (type == ModifierType::Override) {
		voxelutil::overridePlane(volume, pos, face, context.cursorVoxel);
	}
}

void PlaneBrush::preExecute(const BrushContext &context, const voxel::RawVolume *volume) {
	const int thickness = calculateThickness(context);
	ModifierType type = ModifierType::Place;
	if (type == ModifierType::Place) {
		_region =
			voxelutil::extrudePlaneRegion(*volume, _aabbFirstPos, _aabbFace, _hitVoxel, context.cursorVoxel, thickness);
	} else if (type == ModifierType::Erase) {
		_region = voxelutil::erasePlaneRegion(*volume, _aabbFirstPos, _aabbFace, _hitVoxel);
	} else if (type == ModifierType::Override) {
		_region = voxelutil::overridePlaneRegion(*volume, _aabbFirstPos, _aabbFace, context.cursorVoxel);
	}
	_region.grow(1);
}

void PlaneBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						  const BrushContext &context, const voxel::Region &region) {
	const int thickness = calculateThickness(context);
	generateForModifier(wrapper, wrapper.modifierType(), context, _aabbFirstPos, _aabbFace, _hitVoxel, thickness);
}

} // namespace voxedit
