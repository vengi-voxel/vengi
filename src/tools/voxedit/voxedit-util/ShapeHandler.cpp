/**
 * @file
 */

#include "ShapeHandler.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/generator/ShapeGenerator.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/RawVolumeWrapper.h"
#include "math/Random.h"

namespace voxedit {

bool ShapeHandler::scaleCursorShape(const glm::vec3& scale, voxel::RawVolume* cursorVolume) {
	const glm::ivec3 before = _scale;
	_scale *= scale;
	const voxel::Region& r = cursorVolume->region();
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
	const voxel::Region& cursorRegion = cursorVolume->region();
	const glm::ivec3& cursorPos = cursorRegion.getCentre();
	voxel::RawVolumeWrapper wrapper(cursorVolume);
	if (_cursorShape == Shape::Single) {
		cursorVolume->setVoxel(cursorPos, _currentVoxel);
	} else if (_cursorShape == Shape::Dome) {
		const int width = _scale.x;
		const int height = _scale.y;
		const int depth = _scale.z;
		voxel::shape::createDome(wrapper, cursorPos, width, height, depth, _currentVoxel);
	} else if (_cursorShape == Shape::Cone) {
		const int width = _scale.x;
		const int height = _scale.y;
		const int depth = _scale.z;
		voxel::shape::createCone(wrapper, cursorPos, width, height, depth, _currentVoxel);
	} else if (_cursorShape == Shape::Plane) {
		const int width = _scale.x;
		const int height = _scale.y;
		const int depth = _scale.z;
		voxel::shape::createCube(wrapper, cursorPos, width, height, depth, _currentVoxel);
	} else if (_cursorShape == Shape::Circle || _cursorShape == Shape::Cylinder) {
		const int height = _scale.y;
		const int radius = glm::max(1, _scale.x / 2);
		voxel::shape::createCylinder(wrapper, cursorPos, glm::bvec3(false, true, false), radius, height, _currentVoxel);
	} else if (_cursorShape == Shape::Sphere) {
		const int width = _scale.x;
		const int height = _scale.y;
		const int depth = _scale.z;
		voxel::shape::createEllipse(wrapper, cursorPos, width, height, depth, _currentVoxel);
	} else if (_cursorShape == Shape::Torus) {
		const int innerRadius = _scale.x;
		const int outerRadius = _scale.y;
		voxel::shape::createTorus(wrapper, cursorPos, innerRadius, outerRadius, _currentVoxel);
	} else {
		Log::info("Unsupported cursor shape");
	}
}

}
