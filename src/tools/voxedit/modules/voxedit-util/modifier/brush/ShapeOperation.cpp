/**
 * @file
 */

#include "ShapeOperation.h"
#include "core/Log.h"
#include "palette/Palette.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxelgenerator/ShapeGenerator.h"

namespace voxedit {

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
static math::Axis getShapeDimensionForAxis(voxel::FaceNames face, const glm::ivec3 &dimensions, int &width,
												int &height, int &depth) {
	core_assert(face != voxel::FaceNames::Max);
	switch (face) {
	case voxel::FaceNames::PositiveX:
	case voxel::FaceNames::NegativeX:
		width = dimensions.y;
		depth = dimensions.z;
		height = dimensions.x;
		return math::Axis::X;
	case voxel::FaceNames::PositiveY:
	case voxel::FaceNames::NegativeY:
		width = dimensions.x;
		depth = dimensions.z;
		height = dimensions.y;
		return math::Axis::Y;
	case voxel::FaceNames::PositiveZ:
	case voxel::FaceNames::NegativeZ:
		width = dimensions.x;
		depth = dimensions.y;
		height = dimensions.z;
		return math::Axis::Z;
	default:
		break;
	}
	width = 0;
	height = 0;
	depth = 0;
	return math::Axis::None;
}

bool evaluateShape(const ShapeSettings &settings, const ShapeParameters &parameters, ModifierVolumeWrapper &wrapper) {
	const voxel::Region &region = parameters.region;
	const glm::ivec3 &dimensions = region.getDimensionsInVoxels();
	int width = 0;
	int height = 0;
	int depth = 0;
	voxel::FaceNames face = parameters.face;
	if (face == voxel::FaceNames::Max) {
		face = voxel::FaceNames::PositiveX;
	}
	const math::Axis axis = getShapeDimensionForAxis(face, dimensions, width, height, depth);
	const bool negative = voxel::isNegativeFace(face);

	const int axisIdx = math::getIndexForAxis(axis);
	const glm::ivec3 &lc = region.getLowerCorner();
	const glm::ivec3 center = lc + region.getDimensionsInVoxels() / 2;
	glm::ivec3 centerBottom = center;
	centerBottom[axisIdx] = lc[axisIdx];

	const voxel::Voxel &voxel = parameters.voxel;
	if (!voxel::isAir(voxel.getMaterial())) {
		const palette::Palette &palette = wrapper.node().palette();
		if (palette.color(voxel.getColor()).a == 0) {
			Log::warn("Can't place shape with fully transparent color");
			return false;
		}
	}
	switch (settings.shape) {
	case ShapeType::Box:
		voxelgenerator::shape::createCubeNoCenter(wrapper, region.getLowerCorner(), dimensions, voxel);
		break;
	case ShapeType::Torus: {
		voxelgenerator::shape::createTorus(wrapper, center, axis, dimensions, settings.thickness, voxel);
		break;
	}
	case ShapeType::Cylinder: {
		const double radiusX = width / 2.0;
		const double radiusZ = depth / 2.0;
		glm::ivec3 circleCenter = centerBottom;
		glm::ivec3 offset{0};
		offset[axisIdx] = 1;
		for (int i = 0; i < height; ++i) {
			voxelgenerator::shape::createEllipsePlane(wrapper, circleCenter, axis, width, depth, radiusX, radiusZ, voxel);
			circleCenter += offset;
		}
		break;
	}
	case ShapeType::Cone:
		voxelgenerator::shape::createCone(wrapper, centerBottom, axis, negative, width, height, depth, voxel);
		break;
	case ShapeType::Dome:
		voxelgenerator::shape::createDome(wrapper, centerBottom, axis, negative, width, height, depth, voxel);
		break;
	case ShapeType::Ellipse:
		voxelgenerator::shape::createEllipse(wrapper, centerBottom, axis, width, height, depth, voxel);
		break;
	case ShapeType::Circle: {
		const double radiusX = width / 2.0;
		const double radiusZ = depth / 2.0;
		glm::ivec3 circleCenter = centerBottom;
		glm::ivec3 offset{0};
		offset[axisIdx] = 1;
		for (int i = 0; i < height; ++i) {
			voxelgenerator::shape::createEllipseOutline(wrapper, circleCenter, axis, width, depth, radiusX, radiusZ, settings.thickness, voxel);
			circleCenter += offset;
		}
		break;
	}
	case ShapeType::Max:
		Log::warn("Invalid shape type selected - can't perform action");
	}
	return true;
}

} // namespace voxedit
