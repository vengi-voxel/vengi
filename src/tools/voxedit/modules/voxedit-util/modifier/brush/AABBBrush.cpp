/**
 * @file
 */

#include "AABBBrush.h"
#include "command/Command.h"
#include "core/Log.h"
#include "voxedit-util/AxisUtil.h"
#include "voxedit-util/SceneManager.h" // TODO: get rid of this include the reference position is in the BrushContext
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxel/Face.h"

namespace voxedit {

AABBBrush::AABBBrush(BrushType type) : Super(type) {
}

void AABBBrush::construct() {
	// mirroraxisshapebrush, toggleshapebrushcenter, toggleshapebrushsingle
	// mirroraxispaintbrush, togglepaintbrushcenter, togglepaintbrushsingle

	const core::String &cmdName = name().toLower() + "brush";
	command::Command::registerCommand("mirroraxis" + cmdName + "x", [&](const command::CmdArgs &args) {
		toggleMirrorAxis(math::Axis::X, sceneMgr().referencePosition());
	}).setHelp("Mirror along the x axis at the reference position");

	command::Command::registerCommand("mirroraxis" + cmdName + "y", [&](const command::CmdArgs &args) {
		toggleMirrorAxis(math::Axis::Y, sceneMgr().referencePosition());
	}).setHelp("Mirror along the y axis at the reference position");

	command::Command::registerCommand("mirroraxis" + cmdName + "z", [&](const command::CmdArgs &args) {
		toggleMirrorAxis(math::Axis::Z, sceneMgr().referencePosition());
	}).setHelp("Mirror along the z axis at the reference position");

	command::Command::registerCommand("mirroraxis" + cmdName + "none", [&](const command::CmdArgs &args) {
		setMirrorAxis(math::Axis::None, sceneMgr().referencePosition());
	}).setHelp("Disable mirror axis");

	command::Command::registerCommand("toggle" + cmdName + "center", [this](const command::CmdArgs &args) {
		_center ^= true;
	}).setHelp("Toggle center plane building");

	command::Command::registerCommand("toggle" + cmdName + "single", [this](const command::CmdArgs &args) {
		_single ^= true;
		if (!_single) {
			_radius = 0;
		}
	}).setHelp("Toggle single voxel building mode");
}

math::Axis AABBBrush::getShapeDimensionForAxis(voxel::FaceNames face, const glm::ivec3 &dimensions, int &width,
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

void AABBBrush::reset() {
	Super::reset();
	_secondPosValid = false;
	_aabbMode = false;
	_center = false;
	_aabbFace = voxel::FaceNames::Max;
	_aabbFirstPos = glm::ivec3(0);
	_aabbSecondPos = glm::ivec3(0);
	_mirrorAxis = math::Axis::None;
	_mirrorPos = glm::ivec3(0);
}

void AABBBrush::toggleMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos) {
	if (_mirrorAxis == axis) {
		setMirrorAxis(math::Axis::None, mirrorPos);
	} else {
		setMirrorAxis(axis, mirrorPos);
	}
}

bool AABBBrush::setMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos) {
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

glm::ivec3 AABBBrush::applyGridResolution(const glm::ivec3 &inPos, int resolution) const {
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

bool AABBBrush::getMirrorAABB(glm::ivec3 &mins, glm::ivec3 &maxs) const {
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

bool AABBBrush::needsFurtherAction(const BrushContext &context) const {
	if (_radius > 0 || context.lockedAxis != math::Axis::None) {
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

bool AABBBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						const BrushContext &context) {
	const voxel::Region region = calcRegion(context);
	glm::ivec3 minsMirror = region.getLowerCorner();
	glm::ivec3 maxsMirror = region.getUpperCorner();
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		generate(sceneGraph, wrapper, context, region);
	} else {
		Log::debug("Execute mirror action");
		const voxel::Region second(minsMirror, maxsMirror);
		if (voxel::intersects(region, second)) {
			generate(sceneGraph, wrapper, context, voxel::Region(region.getLowerCorner(), maxsMirror));
		} else {
			generate(sceneGraph, wrapper, context, region);
			generate(sceneGraph, wrapper, context, second);
		}
	}
	markDirty();
	return true;
}

glm::ivec3 AABBBrush::currentCursorPosition(const glm::ivec3 &cursorPosition) const {
	glm::ivec3 pos = cursorPosition;
	if (_secondPosValid) {
		if (_radius > 0) {
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

bool AABBBrush::wantAABB() const {
	return !_single;
}

bool AABBBrush::start(const BrushContext &context) {
	if (_aabbMode) {
		return false;
	}

	// the order here matters - don't change _aabbMode earlier here
	_aabbFirstPos = applyGridResolution(context.cursorPosition, context.gridResolution);
	_secondPosValid = false;
	_aabbMode = wantAABB();
	_aabbFace = context.cursorFace;
	markDirty();
	return true;
}

bool AABBBrush::active() const {
	return _aabbMode;
}

bool AABBBrush::aborted(const BrushContext &context) const {
	return _aabbFace == voxel::FaceNames::Max && context.lockedAxis == math::Axis::None;
}

void AABBBrush::step(const BrushContext &context) {
	if (!_aabbMode || _radius > 0 || context.lockedAxis != math::Axis::None) {
		return;
	}
	_aabbSecondPos = currentCursorPosition(context.cursorPosition);
	_aabbFirstPos = applyGridResolution(_aabbFirstPos, context.gridResolution);
	_secondPosValid = true;
	markDirty();
}

void AABBBrush::stop(const BrushContext &context) {
	_secondPosValid = false;
	_aabbMode = false;
	_aabbFace = voxel::FaceNames::Max;
	markDirty();
}

voxel::Region AABBBrush::calcRegion(const BrushContext &context) const {
	const glm::ivec3 &pos = currentCursorPosition(context.cursorPosition);
	if (!_single && _center) {
		const glm::ivec3 &first = applyGridResolution(_aabbFirstPos, context.gridResolution);
		const glm::ivec3 &delta = glm::abs(pos - first);
		return voxel::Region(first - delta, first + delta);
	}
	if (_radius > 0) {
		const glm::ivec3 &first = _single ? pos : applyGridResolution(_aabbFirstPos, context.gridResolution);
		const glm::ivec3 delta(_radius);
		return voxel::Region(first - delta, first + delta);
	}

	const int size = context.gridResolution;
	const glm::ivec3 &first = _single ? pos : applyGridResolution(_aabbFirstPos, context.gridResolution);
	const glm::ivec3 &mins = (glm::min)(first, pos);
	const glm::ivec3 &maxs = (glm::max)(first, pos) + (size - 1);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
