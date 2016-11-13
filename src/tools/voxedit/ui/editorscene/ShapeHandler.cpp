#include "ShapeHandler.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/generator/ShapeGenerator.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "core/Random.h"
#include "voxel/generator/TreeGenerator.h"

namespace voxedit {

// TODO: scale via s x v (scale, axis, value)
bool ShapeHandler::setCursorShape(Shape type, voxel::RawVolume* cursorVolume, bool force) {
	if (_cursorShape == type && !force) {
		return false;
	}
	_cursorShape = type;
	_cursorShapeState = ShapeState::New;
	const glm::ivec3& center = cursorVolume->getEnclosingRegion().getCentre();
	int width = 3;
	int height = 3;
	int depth = 3;
	if (_cursorShape == Shape::Single) {
		cursorVolume->clear();
		return true;
	} else if (_cursorShape == Shape::Dome) {
		cursorVolume->clear();
		voxel::shape::createDome(*cursorVolume, center, width, height, depth, _currentVoxel);
	} else if (_cursorShape == Shape::Cone) {
		cursorVolume->clear();
		voxel::shape::createCone(*cursorVolume, center, width, height, depth, _currentVoxel);
	} else if (_cursorShape == Shape::Plane) {
		cursorVolume->clear();
		voxel::shape::createPlane(*cursorVolume, center, width, depth, _currentVoxel);
	} else if (_cursorShape == Shape::Circle) {
		const double radius = 3.0;
		cursorVolume->clear();
		voxel::shape::createCirclePlane(*cursorVolume, center, width, depth, radius, _currentVoxel);
	} else if (_cursorShape == Shape::Sphere) {
		cursorVolume->clear();
		core::Random random;
		voxel::tree::addTree(*cursorVolume, center, voxel::TreeType::PINE, 10, 4, 20, 20, 20, random);
	} else {
		Log::info("Unsupported cursor shape");
	}
	_cursorShapeState = ShapeState::Created;
	return false;
}

bool ShapeHandler::placeCursor(voxel::RawVolume* modelVolume, const voxel::RawVolume* cursorPositionVolume) {
	return voxel::mergeRawVolumesSameDimension(modelVolume, cursorPositionVolume) > 0;
}

}
