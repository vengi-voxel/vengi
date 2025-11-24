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
		const core::String &cmd = "shape" + typeStr;
		command::Command::registerCommand(cmd.c_str(), [&, type](const command::CmdArgs &args) {
			setShapeType((ShapeType)type);
		}).setHelp(_("Change the modifier shape type"));
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
	const double size = (glm::max)(width, depth);
	const bool negative = voxel::isNegativeFace(face);

	const int axisIdx = math::getIndexForAxis(axis);
	const glm::ivec3 &center = region.getCenter();
	glm::ivec3 centerBottom = center;
	centerBottom[axisIdx] = region.getLowerCorner()[axisIdx];

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
		const double minorRadius = size / 5.0;
		const double majorRadius = size / 2.0 - minorRadius;
		voxelgenerator::shape::createTorus(wrapper, center, minorRadius, majorRadius, voxel);
		break;
	}
	case ShapeType::Cylinder: {
		const int radius = (int)glm::round(size / 2.0);
		voxelgenerator::shape::createCylinder(wrapper, centerBottom, axis, radius, height, voxel);
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
	case ShapeType::Max:
		Log::warn("Invalid shape type selected - can't perform action");
	}
}

void ShapeBrush::reset() {
	Super::reset();
	_shapeType = ShapeType::AABB;
}

} // namespace voxedit
