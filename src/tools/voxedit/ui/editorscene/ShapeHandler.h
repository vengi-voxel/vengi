#pragma once

#include "core/GLM.h"
#include "Shape.h"
#include "voxel/polyvox/Voxel.h"

namespace voxel {
class RawVolume;
}

namespace voxedit {

class ShapeHandler {
private:
	voxel::Voxel _currentVoxel;
	Shape _cursorShape = Shape::Single;
	glm::ivec3 _scale;
	void createCursorShape(voxel::RawVolume* cursorVolume);
public:
	bool scaleCursorShape(const glm::vec3& scale, voxel::RawVolume* cursorVolume);
	bool setCursorShape(Shape type, voxel::RawVolume* cursorVolume, bool force);
	bool placeCursor(voxel::RawVolume* modelVolume, const voxel::RawVolume* cursorPositionVolume);

	const voxel::Voxel& currentVoxel() const;
	Shape cursorShape() const;
	void setVoxelType(voxel::VoxelType type);
};

inline const voxel::Voxel& ShapeHandler::currentVoxel() const {
	return _currentVoxel;
}

inline Shape ShapeHandler::cursorShape() const {
	return _cursorShape;
}

inline void ShapeHandler::setVoxelType(voxel::VoxelType type) {
	_currentVoxel = voxel::createVoxel(type);
}

}
