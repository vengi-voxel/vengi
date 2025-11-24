/**
 * @file
 */

#pragma once

#include "../ShapeType.h"
#include "AABBBrush.h"

namespace voxedit {

/**
 * @brief A brush that can generate several different shapes
 * @sa ShapeType enum
 * @ingroup Brushes
 */
class ShapeBrush : public AABBBrush {
private:
	using Super = AABBBrush;

protected:
	math::Axis getShapeDimensionForAxis(voxel::FaceNames face, const glm::ivec3 &dimensions, int &width, int &height,
										int &depth) const;
	ShapeType _shapeType = ShapeType::AABB;
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
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
