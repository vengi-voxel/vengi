/**
 * @file
 */

#include "ShapeBrush.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "core/Log.h"
#include "palette/Palette.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Face.h"
#include "voxelgenerator/ShapeGenerator.h"

namespace voxedit {

void ShapeBrush::construct() {
	Super::construct();
	for (int type = ShapeType::Min; type < ShapeType::Max; ++type) {
		const core::String &typeStr = core::String::lower(ShapeTypeStr[type]);
		const core::String &cmd = "shape" + typeStr; // shapeaabb, ...
		const core::String &help = core::String::format(_("Change the shape type to %s"), _(ShapeTypeStr[type]));
		command::Command::registerCommand(cmd.c_str())
			.setHandler([&, type](const command::CommandArgs &args) {
				setShapeType((ShapeType)type);
			}).setHelp(help);
	}
}

ShapeType ShapeBrush::shapeType() const {
	return _shapeType;
}

void ShapeBrush::setShapeType(ShapeType type) {
	_shapeType = type;
	markDirty();
}

math::Axis ShapeBrush::getShapeDimensionForAxis(voxel::FaceNames face, const glm::ivec3 &dimensions, int &width,
												int &height, int &depth) const {
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

void ShapeBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region) {
	const glm::ivec3 &dimensions = region.getDimensionsInVoxels();
	int width = 0;
	int height = 0;
	int depth = 0;
	voxel::FaceNames face = _aabbFace;
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

	const voxel::Voxel &voxel = ctx.cursorVoxel;
	if (!voxel::isAir(voxel.getMaterial())) {
		const palette::Palette &palette = wrapper.node().palette();
		if (palette.color(voxel.getColor()).a == 0) {
			Log::warn("Can't place shape with fully transparent color");
			return;
		}
	}
	switch (_shapeType) {
	case ShapeType::AABB:
		voxelgenerator::shape::createCubeNoCenter(wrapper, region.getLowerCorner(), dimensions, voxel);
		break;
	case ShapeType::Torus: {
		voxelgenerator::shape::createTorus(wrapper, center, axis, dimensions, voxel);
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
			voxelgenerator::shape::createEllipseOutline(wrapper, circleCenter, axis, width, depth, radiusX, radiusZ, _thickness, voxel);
			circleCenter += offset;
		}
		break;
	}
	case ShapeType::Max:
		Log::warn("Invalid shape type selected - can't perform action");
	}
}

int ShapeBrush::thickness() const {
	return _thickness;
}

void ShapeBrush::setThickness(int thickness) {
	_thickness = core_max(1, thickness);
	markDirty();
}

void ShapeBrush::reset() {
	Super::reset();
	_shapeType = ShapeType::AABB;
	_thickness = 1;
}

} // namespace voxedit
