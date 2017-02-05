#include "ShapeHandler.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/generator/ShapeGenerator.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "core/Random.h"

namespace voxedit {

bool ShapeHandler::scaleCursorShape(const glm::vec3& scale, voxel::RawVolume* cursorVolume) {
	const glm::ivec3 before = _scale;
	_scale *= scale;
	const voxel::Region& r = cursorVolume->getRegion();
	_scale = glm::clamp(_scale, glm::ivec3(1), r.getDimensionsInVoxels());
	if (_scale == before) {
		return false;
	}
	createCursorShape(cursorVolume);
	return true;
}

bool ShapeHandler::setCursorShape(Shape type, voxel::RawVolume* cursorVolume, bool force) {
	if (_cursorShape == type && !force) {
		return false;
	}
	_cursorShape = type;
	_scale = glm::ivec3(3);
	createCursorShape(cursorVolume);
	return true;
}

void ShapeHandler::createCursorShape(voxel::RawVolume* cursorVolume) {
	cursorVolume->clear();
	const voxel::Region& cursorRegion = cursorVolume->getRegion();
	const glm::ivec3& cursorPos = cursorRegion.getCentre();
	if (_cursorShape == Shape::Single) {
		cursorVolume->setVoxel(cursorPos, _currentVoxel);
	} else if (_cursorShape == Shape::Dome) {
		voxel::shape::createDome(*cursorVolume, cursorPos, _scale.x, _scale.y, _scale.z, _currentVoxel);
	} else if (_cursorShape == Shape::Cone) {
		voxel::shape::createCone(*cursorVolume, cursorPos, _scale.x, _scale.y, _scale.z, _currentVoxel);
	} else if (_cursorShape == Shape::Plane) {
		voxel::shape::createPlane(*cursorVolume, cursorPos, _scale.x, _scale.z, _currentVoxel);
	} else if (_cursorShape == Shape::Circle) {
		const double radius = 3.0;
		voxel::shape::createCirclePlane(*cursorVolume, cursorPos, _scale.x, _scale.z, radius, _currentVoxel);
	} else if (_cursorShape == Shape::Sphere) {
		voxel::shape::createEllipse(*cursorVolume, cursorPos, _scale.x, _scale.y, _scale.z, _currentVoxel);
	} else {
		Log::info("Unsupported cursor shape");
	}
}

bool ShapeHandler::placeCursor(voxel::RawVolume* modelVolume, const voxel::RawVolume* cursorVolume, const glm::ivec3& pos, voxel::Region *region) {
	const voxel::Region& cursorRegion = cursorVolume->getRegion();
	const glm::ivec3 mins = -cursorRegion.getCentre() + pos;
	const glm::ivec3 maxs = mins + cursorRegion.getDimensionsInCells();
	const voxel::Region destRegion(mins, maxs);
	if (voxel::mergeRawVolumes(modelVolume, cursorVolume, destRegion, cursorRegion) <= 0) {
		return false;
	}
	if (region != nullptr) {
		*region = destRegion;
	}
	return true;
}

}
