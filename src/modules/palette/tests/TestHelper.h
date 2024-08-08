/**
 * @file
 */

#pragma once

#include "palette/Palette.h"
#include <gtest/gtest.h>

namespace palette {

inline ::std::ostream &operator<<(::std::ostream &os, const palette::Palette &palette) {
	return os << palette::Palette::print(palette).c_str();
}

inline ::std::ostream &operator<<(::std::ostream &os, const palette::Material &material) {
	os << "Material: " << (int)material.type << " ";
	for (uint32_t i = 0; i < palette::MaterialProperty::MaterialMax - 1; ++i) {
		if (!material.has((palette::MaterialProperty)i)) {
			continue;
		}
		os << palette::MaterialPropertyNames[i] << ": " << material.value((palette::MaterialProperty)i) << ", ";
	}
	return os;
}

} // namespace palette
