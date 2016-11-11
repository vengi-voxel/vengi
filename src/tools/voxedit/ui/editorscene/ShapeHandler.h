#pragma once

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
	enum class ShapeState {
		/** just switched the shape type */
		New,
		/** clicked into the volume but didn't yet configure the cursor shape - e.g. modify the circle size */
		Configure,
		/** fully created the cursor volume, can be placed on click */
		Created
	};
	ShapeState _cursorShapeState = ShapeState::New;
public:
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
