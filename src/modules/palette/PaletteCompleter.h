/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"

namespace palette {

/**
 * @brief Command completer
 *
 * @sa command::Command
 */
inline auto paletteCompleter() {
	return [&](const core::String &str, core::DynamicArray<core::String> &matches) -> int {
		int i = 0;
		for (i = 0; i < lengthof(palette::Palette::builtIn); ++i) {
			if (core::string::startsWith(palette::Palette::builtIn[i], str.c_str())) {
				matches.push_back(palette::Palette::builtIn[i]);
			}
		}
		return i;
	};
}

} // namespace palette
