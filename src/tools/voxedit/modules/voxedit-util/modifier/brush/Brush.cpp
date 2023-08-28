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

} // namespace voxedit
