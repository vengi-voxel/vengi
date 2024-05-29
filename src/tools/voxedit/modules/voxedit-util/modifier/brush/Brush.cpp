/**
 * @file
 */

#include "Brush.h"
#include "voxedit-util/AxisUtil.h"

namespace voxedit {

void Brush::reset() {
	_brushClamping = false;
	_mirrorAxis = math::Axis::None;
	_mirrorPos = glm::ivec3(0);
	markDirty();
}

void Brush::update(const BrushContext &ctx, double nowSeconds) {
}

void Brush::setBrushClamping(bool brushClamping) {
	_brushClamping = brushClamping;
}

bool Brush::brushClamping() const {
	return _brushClamping;
}

void Brush::toggleMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos) {
	if (_mirrorAxis == axis) {
		setMirrorAxis(math::Axis::None, mirrorPos);
	} else {
		setMirrorAxis(axis, mirrorPos);
	}
}

bool Brush::setMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos) {
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

bool Brush::getMirrorAABB(glm::ivec3 &mins, glm::ivec3 &maxs) const {
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

ModifierType Brush::modifierType(ModifierType type) const {
	ModifierType newType = type & _supportedModifiers;
	if (newType == ModifierType::None) {
		newType = _defaultModifier;
	}
	return newType;
}

/**
 * @brief Determine whether the brush should get rendered
 */
bool Brush::active() const {
	return true;
}

bool Brush::init() {
	return true;
}

void Brush::shutdown() {
}

} // namespace voxedit
