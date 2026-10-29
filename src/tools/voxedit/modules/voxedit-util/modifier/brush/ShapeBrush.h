/**
 * @file
 */

#pragma once

#include "../ShapeType.h"
#include "AABBBrush.h"
#include "ShapeOperation.h"

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
 * - **Circle**: Hollow cylinder (tube) with configurable wall thickness
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
	voxel::FaceNames _lastCursorFace = voxel::FaceNames::Max;
	bool _lastFaceCursorPreview = true;

protected:
	ShapeType _shapeType = ShapeType::Box; ///< Current shape being generated
	int _thickness = 1; ///< Wall thickness for hollow shapes (e.g., Circle, Torus)

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
		setStrokeMode();
	}
	virtual ~ShapeBrush() = default;
	void construct() override;
	void reset() override;
	void update(const BrushContext &ctx, double nowSeconds) override;

	bool isSimplePreview() const override {
		return shapeType() == ShapeType::Box && !preferFaceCursorPreview();
	}

	bool preferFaceCursorPreview() const override;

	/**
	 * @return The currently active shape type
	 */
	ShapeType shapeType() const;
	ShapeSettings settings() const;

	/**
	 * @return The wall thickness for hollow shapes
	 */
	int thickness() const;

	/**
	 * @brief Set the wall thickness for hollow shapes
	 */
	void setThickness(int thickness);
};

} // namespace voxedit
