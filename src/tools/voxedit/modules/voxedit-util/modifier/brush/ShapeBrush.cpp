/**
 * @file
 */

#include "ShapeBrush.h"
#include "command/Command.h"
#include "core/Log.h"
#include "voxedit-util/AxisUtil.h"
#include "voxedit-util/SceneManager.h" // TODO: get rid of this include the refernece position is in the BrushContext
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Face.h"
#include "voxelgenerator/ShapeGenerator.h"

namespace voxedit {

void ShapeBrush::construct() {
	command::Command::registerCommand("mirroraxisx", [&](const command::CmdArgs &args) {
		toggleMirrorAxis(math::Axis::X, sceneMgr().referencePosition());
	}).setHelp("Mirror around the x axis");

	command::Command::registerCommand("mirroraxisy", [&](const command::CmdArgs &args) {
		toggleMirrorAxis(math::Axis::Y, sceneMgr().referencePosition());
	}).setHelp("Mirror around the y axis");

	command::Command::registerCommand("mirroraxisz", [&](const command::CmdArgs &args) {
		toggleMirrorAxis(math::Axis::Z, sceneMgr().referencePosition());
	}).setHelp("Mirror around the z axis");

	command::Command::registerCommand("mirroraxisnone", [&](const command::CmdArgs &args) {
		setMirrorAxis(math::Axis::None, sceneMgr().referencePosition());
	}).setHelp("Disable mirror axis");

	command::Command::registerCommand("toggleshapebrushcenter", [this](const command::CmdArgs &args) {
		_center ^= true;
	}).setHelp("Toggle center plane building");

	command::Command::registerCommand("toggleshapebrushsingle", [this](const command::CmdArgs &args) {
		_single ^= true;
	}).setHelp("Toggle single voxel building mode");

	for (int type = ShapeType::Min; type < ShapeType::Max; ++type) {
		const core::String &typeStr = core::String::lower(ShapeTypeStr[type]);
		const core::String &cmd = "shape" + typeStr;
		command::Command::registerCommand(cmd.c_str(), [&, type](const command::CmdArgs &args) {
			setShapeType((ShapeType)type);
		}).setHelp("Change the modifier shape type");
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

bool ShapeBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						  const voxel::Region &region, const voxel::Voxel &voxel) {
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
		return false;
	}
	return true;
}

void ShapeBrush::reset() {
	Super::reset();
	_shapeType = ShapeType::AABB;
	_secondPosValid = false;
	_aabbMode = false;
	_aabbFace = voxel::FaceNames::Max;
	_aabbFirstPos = glm::ivec3(0);
	_aabbSecondPos = glm::ivec3(0);
	_center = false;
	_mirrorAxis = math::Axis::None;
	_mirrorPos = glm::ivec3(0);
}

void ShapeBrush::toggleMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos) {
	if (_mirrorAxis == axis) {
		setMirrorAxis(math::Axis::None, mirrorPos);
	} else {
		setMirrorAxis(axis, mirrorPos);
	}
}

bool ShapeBrush::setMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos) {
	if (_mirrorAxis == axis) {
		if (_mirrorPos != mirrorPos) {
			_mirrorPos = mirrorPos;
			return true;
		}
		return false;
	}
	_mirrorPos = mirrorPos;
	_mirrorAxis = axis;
	markDirty();
	return true;
}

glm::ivec3 ShapeBrush::applyGridResolution(const glm::ivec3 &inPos, int resolution) const {
	glm::ivec3 pos = inPos;
	if (pos.x % resolution != 0) {
		pos.x = (pos.x / resolution) * resolution;
	}
	if (pos.y % resolution != 0) {
		pos.y = (pos.y / resolution) * resolution;
	}
	if (pos.z % resolution != 0) {
		pos.z = (pos.z / resolution) * resolution;
	}
	return pos;
}

bool ShapeBrush::getMirrorAABB(glm::ivec3 &mins, glm::ivec3 &maxs) const {
	math::Axis mirrorAxis = _mirrorAxis;
	if (mirrorAxis == math::Axis::None) {
		return false;
	}
	const int index = getIndexForMirrorAxis(mirrorAxis);
	int deltaMaxs = _mirrorPos[index] - maxs[index] - 1;
	deltaMaxs *= 2;
	deltaMaxs += (maxs[index] - mins[index] + 1);
	mins[index] += deltaMaxs;
	maxs[index] += deltaMaxs;
	return true;
}

bool ShapeBrush::needsFurtherAction(const BrushContext &context) const {
	if (_fixedRegion || context.lockedAxis != math::Axis::None) {
		return false;
	}
	const voxel::Region &region = calcRegion(context);
	const glm::ivec3 &delta = region.getDimensionsInVoxels();
	int greater = 0;
	int equal = 0;
	for (int i = 0; i < 3; ++i) {
		if (delta[i] > context.gridResolution) {
			++greater;
		} else if (delta[i] == context.gridResolution) {
			++equal;
		}
	}
	return greater == 2 && equal == 1;
}

bool ShapeBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						  const BrushContext &context) {
	const voxel::Region region = calcRegion(context);
	glm::ivec3 minsMirror = region.getLowerCorner();
	glm::ivec3 maxsMirror = region.getUpperCorner();
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		generate(sceneGraph, wrapper, region, context.cursorVoxel);
	} else {
		Log::debug("Execute mirror action");
		const voxel::Region second(minsMirror, maxsMirror);
		if (voxel::intersects(region, second)) {
			generate(sceneGraph, wrapper, voxel::Region(region.getLowerCorner(), maxsMirror), context.cursorVoxel);
		} else {
			generate(sceneGraph, wrapper, region, context.cursorVoxel);
			generate(sceneGraph, wrapper, second, context.cursorVoxel);
		}
	}
	markDirty();
	return true;
}

glm::ivec3 ShapeBrush::currentCursorPosition(const glm::ivec3 &cursorPosition) const {
	glm::ivec3 pos = cursorPosition;
	if (_secondPosValid) {
		if (_fixedRegion) {
			return _aabbSecondPos;
		}
		switch (_aabbFace) {
		case voxel::FaceNames::PositiveX:
		case voxel::FaceNames::NegativeX:
			pos.y = _aabbSecondPos.y;
			pos.z = _aabbSecondPos.z;
			break;
		case voxel::FaceNames::PositiveY:
		case voxel::FaceNames::NegativeY:
			pos.x = _aabbSecondPos.x;
			pos.z = _aabbSecondPos.z;
			break;
		case voxel::FaceNames::PositiveZ:
		case voxel::FaceNames::NegativeZ:
			pos.x = _aabbSecondPos.x;
			pos.y = _aabbSecondPos.y;
			break;
		default:
			break;
		}
	}
	return pos;
}

bool ShapeBrush::start(const BrushContext &context) {
	if (_aabbMode) {
		return false;
	}

	// the order here matters - don't change _aabbMode earlier here
	_aabbFirstPos = applyGridResolution(context.cursorPosition, context.gridResolution);
	_fixedRegion = false;
	_secondPosValid = false;
	_aabbMode = !_single;
	_aabbFace = context.cursorFace;
	markDirty();
	return true;
}

bool ShapeBrush::active() const {
	return _aabbMode;
}

bool ShapeBrush::aborted(const BrushContext &context) const {
	return _aabbFace == voxel::FaceNames::Max && context.lockedAxis == math::Axis::None;
}

void ShapeBrush::step(const BrushContext &context) {
	if (!_aabbMode || _fixedRegion || context.lockedAxis != math::Axis::None) {
		return;
	}
	_aabbSecondPos = currentCursorPosition(context.cursorPosition);
	_aabbFirstPos = applyGridResolution(_aabbFirstPos, context.gridResolution);
	_secondPosValid = true;
	markDirty();
}

void ShapeBrush::stop(const BrushContext &context) {
	_secondPosValid = false;
	_aabbMode = false;
	_aabbFace = voxel::FaceNames::Max;
	markDirty();
}

voxel::Region ShapeBrush::calcRegion(const BrushContext &context) const {
	const glm::ivec3 &pos = currentCursorPosition(context.cursorPosition);
		if (!_single && _center) {
		const glm::ivec3 &first = applyGridResolution(_aabbFirstPos, context.gridResolution);
		const glm::ivec3 &delta = glm::abs(pos - first);
		return voxel::Region(first - delta, first + delta);
	}

	const int size = context.gridResolution;
	const glm::ivec3 &first = _single ? pos : applyGridResolution(_aabbFirstPos, context.gridResolution);
	const glm::ivec3 &mins = (glm::min)(first, pos);
	const glm::ivec3 &maxs = (glm::max)(first, pos) + (size - 1);
	return voxel::Region(mins, maxs);
}

void ShapeBrush::setRegion(const voxel::Region &region, voxel::FaceNames face) {
	_aabbFirstPos = region.getLowerCorner();
	_aabbSecondPos = region.getUpperCorner();
	_secondPosValid = true;
	_aabbMode = true;
	_fixedRegion = true;
	_aabbFace = face;
	markDirty();
}

} // namespace voxedit
