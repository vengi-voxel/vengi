/**
 * @file
 */

#include "Types.h"
#include "Util.h"

size_t Layout::typeSize(const Variable& v) const {
	if (blockLayout == BlockLayout::std140) {
		return util::std140Size(v);
	}
	return 0;
}

int Layout::typeAlign(const Variable& v) const {
	if (blockLayout == BlockLayout::std140) {
		return util::std140Align(v);
	}
	return 0;
}
