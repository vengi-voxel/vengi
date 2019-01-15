/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "Shape.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/polyvox/Region.h"

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

	const voxel::Voxel& cursorVoxel() const;
	Shape cursorShape() const;
	void setCursorVoxel(const voxel::Voxel& type);
};

inline const voxel::Voxel& ShapeHandler::cursorVoxel() const {
	return _currentVoxel;
}

inline Shape ShapeHandler::cursorShape() const {
	return _cursorShape;
}

inline void ShapeHandler::setCursorVoxel(const voxel::Voxel& type) {
	_currentVoxel = type;
}

}
