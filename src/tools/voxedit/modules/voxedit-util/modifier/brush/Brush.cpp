/**
 * @file
 */

#include "Brush.h"

namespace voxedit {

void Brush::markDirty() {
	_dirty = true;
}

void Brush::reset() {
	markDirty();
}

bool Brush::dirty() const {
	return _dirty;
}

void Brush::markClean() {
	_dirty = false;
}

void Brush::update(const BrushContext &ctx, double nowSeconds) {
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
