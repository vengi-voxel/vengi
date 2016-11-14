#include "ShapeHandler.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/generator/ShapeGenerator.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "core/Random.h"

namespace voxedit {

bool ShapeHandler::scaleCursorShape(const glm::vec3& scale, voxel::RawVolume* cursorVolume) {
	const glm::ivec3 before = _scale;
	_scale *= scale;
	const voxel::Region& r = cursorVolume->getEnclosingRegion();
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
	const voxel::Region& cursorRegion = cursorVolume->getEnclosingRegion();
	glm::ivec3 cursorPos = cursorRegion.getCentre();
	if (_cursorShape == Shape::Single) {
		cursorVolume->clear();
		cursorVolume->setVoxel(cursorPos, _currentVoxel);
	} else if (_cursorShape == Shape::Dome) {
		cursorVolume->clear();
		voxel::shape::createDome(*cursorVolume, cursorPos, _scale.x, _scale.y, _scale.z, _currentVoxel);
	} else if (_cursorShape == Shape::Cone) {
		cursorVolume->clear();
		voxel::shape::createCone(*cursorVolume, cursorPos, _scale.x, _scale.y, _scale.z, _currentVoxel);
	} else if (_cursorShape == Shape::Plane) {
		cursorVolume->clear();
		voxel::shape::createPlane(*cursorVolume, cursorPos, _scale.x, _scale.z, _currentVoxel);
	} else if (_cursorShape == Shape::Circle) {
		const double radius = 3.0;
		cursorVolume->clear();
		voxel::shape::createCirclePlane(*cursorVolume, cursorPos, _scale.x, _scale.z, radius, _currentVoxel);
	} else if (_cursorShape == Shape::Sphere) {
		cursorVolume->clear();
		voxel::shape::createEllipse(*cursorVolume, cursorPos, _scale.x, _scale.x, _scale.x, _currentVoxel);
	} else {
		Log::info("Unsupported cursor shape");
	}
}

bool ShapeHandler::placeCursor(voxel::RawVolume* modelVolume, const voxel::RawVolume* cursorPositionVolume) {
	return voxel::mergeRawVolumesSameDimension(modelVolume, cursorPositionVolume) > 0;
}

}
