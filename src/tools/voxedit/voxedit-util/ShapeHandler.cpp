#include "ShapeHandler.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/generator/ShapeGenerator.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/RawVolumeWrapper.h"
#include "core/Random.h"

namespace voxedit {

bool ShapeHandler::scaleCursorShape(const glm::vec3& scale, voxel::RawVolume* cursorVolume) {
	const glm::ivec3 before = _scale;
	_scale *= scale;
	const voxel::Region& r = cursorVolume->getRegion();
	_scale = glm::clamp(_scale, glm::ivec3(1), r.getDimensionsInVoxels() * 10);
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
	voxel::RawVolumeWrapper wrapper(cursorVolume);
	if (_cursorShape == Shape::Single) {
		cursorVolume->setVoxel(cursorPos, _currentVoxel);
	} else if (_cursorShape == Shape::Dome) {
		voxel::shape::createDome(wrapper, cursorPos, _scale.x, _scale.y, _scale.z, _currentVoxel);
	} else if (_cursorShape == Shape::Cone) {
		voxel::shape::createCone(wrapper, cursorPos, _scale.x, _scale.y, _scale.z, _currentVoxel);
	} else if (_cursorShape == Shape::Plane) {
		voxel::shape::createPlane(wrapper, cursorPos, _scale.x, _scale.z, _currentVoxel);
	} else if (_cursorShape == Shape::Circle) {
		const double radius = 3.0;
		voxel::shape::createCirclePlane(wrapper, cursorPos, _scale.x, _scale.z, radius, _currentVoxel);
	} else if (_cursorShape == Shape::Sphere) {
		voxel::shape::createEllipse(wrapper, cursorPos, _scale.x, _scale.y, _scale.z, _currentVoxel);
	} else if (_cursorShape == Shape::Torus) {
		const int innerRadius = 3;
		const int outerRadius = 10;
		voxel::shape::createTorus(wrapper, cursorPos, innerRadius, outerRadius, _currentVoxel);
	} else if (_cursorShape == Shape::Cylinder) {
		voxel::shape::createCylinder(wrapper, cursorPos, glm::bvec3(0, 1, 0), _scale.x, _scale.y, _currentVoxel);
	} else {
		Log::info("Unsupported cursor shape");
	}
}

}
