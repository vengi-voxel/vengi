/**
 * @file
 */

#include "Brush.h"

namespace voxedit {

void Brush::reset() {
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
