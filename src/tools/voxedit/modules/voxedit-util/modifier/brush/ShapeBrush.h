/**
 * @file
 */

#pragma once

#include "../ShapeType.h"
#include "AABBBrush.h"

namespace voxedit {

class ShapeBrush : public AABBBrush {
private:
	using Super = AABBBrush;
protected:
	math::Axis getShapeDimensionForAxis(voxel::FaceNames face, const glm::ivec3 &dimensions, int &width, int &height,
										int &depth) const;
	ShapeType _shapeType = ShapeType::AABB;
	bool generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &context,
				  const voxel::Region &region) override;
	void setShapeType(ShapeType type);

public:
	ShapeBrush() : Super(BrushType::Shape) {
	}
	virtual ~ShapeBrush() = default;
	void construct() override;
	void reset() override;

	ShapeType shapeType() const;
};

} // namespace voxedit
