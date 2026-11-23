/**
 * @file
 */

#pragma once

#include "core/Bits.h"
#include "core/String.h"

namespace core {

template<typename TYPE>
core::String toBitString(TYPE val) {
	size_t l = sizeof(val) * 8;
	core::String str(l, '0');
	for (size_t i = 0; i < l; ++i) {
		if ((val & (1 << i)) != 0) {
			str[l - 1 - i] = '1';
		}
	}
	return str;
}

}
