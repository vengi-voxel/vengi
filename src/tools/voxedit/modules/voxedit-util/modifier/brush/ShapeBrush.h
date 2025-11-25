/**
 * @file
 */

#pragma once

#include "../ShapeType.h"
#include "AABBBrush.h"

namespace voxedit {

/**
 * @brief A brush that generates geometric shapes within an AABB
 *
 * The ShapeBrush creates various 3D geometric primitives by filling or outlining
 * voxels within the region defined by the AABB. It supports:
 *
 * - **AABB** (Cube): Hollow or filled rectangular volume
 * - **Torus**: Donut shape with configurable radii
 * - **Cylinder**: Circular column along an axis
 * - **Cone**: Tapered cylinder
 * - **Dome**: Half-ellipse/sphere
 * - **Ellipse**: Stretched sphere
 *
 * The shape is oriented based on which face was hit when starting the AABB. The
 * face normal determines the "up" direction for shapes like cones and cylinders.
 *
 * @sa ShapeType enum for all available shapes
 * @sa AABBBrush for the AABB spanning behavior
 * @ingroup Brushes
 */
class ShapeBrush : public AABBBrush {
private:
	using Super = AABBBrush;

protected:
	/**
	 * @brief Determine the shape dimensions based on face orientation
	 *
	 * Extracts width, height, and depth from the AABB dimensions based on which
	 * face was hit. This allows shapes to be oriented correctly relative to the
	 * surface the user was interacting with.
	 *
	 * @param[in] face The face that was hit when starting the AABB
	 * @param[in] dimensions The AABB dimensions
	 * @param[out] width Shape width perpendicular to face normal
	 * @param[out] height Shape height along face normal
	 * @param[out] depth Shape depth perpendicular to face normal and width
	 * @return The axis aligned with the face normal
	 */
	math::Axis getShapeDimensionForAxis(voxel::FaceNames face, const glm::ivec3 &dimensions, int &width, int &height,
										int &depth) const;

	ShapeType _shapeType = ShapeType::AABB; ///< Current shape being generated

	/**
	 * @brief Generate the selected shape within the given region
	 *
	 * Dispatches to the appropriate shape generator based on _shapeType, converting
	 * the AABB dimensions and orientation into shape-specific parameters.
	 */
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

	/**
	 * @brief Change the active shape type
	 * @param[in] type The new shape to generate
	 */
	void setShapeType(ShapeType type);

public:
	ShapeBrush() : Super(BrushType::Shape) {
	}
	virtual ~ShapeBrush() = default;
	void construct() override;
	void reset() override;

	/**
	 * @return The currently active shape type
	 */
	ShapeType shapeType() const;
};

} // namespace voxedit
