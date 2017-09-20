/**
 * @file
 */

#include "Types.h"
#include "Util.h"

std::string Layout::typeAlign(const Variable& v) const {
	switch (blockLayout) {
	default:
	case BlockLayout::std140:
		return util::std140Align(v);
	case BlockLayout::std430:
		return util::std430Align(v);
	}
}

size_t Layout::typeSize(const Variable& v) const {
	switch (blockLayout) {
	default:
	case BlockLayout::std140:
		return util::std140Size(v);
	case BlockLayout::std430:
		return util::std430Size(v);
	}
}

std::string Layout::typePadding(const Variable& v, int& padding) const {
	switch (blockLayout) {
	default:
	case BlockLayout::std140:
		return util::std140Padding(v, padding);
	case BlockLayout::std430:
		return util::std430Padding(v, padding);
	}
}
