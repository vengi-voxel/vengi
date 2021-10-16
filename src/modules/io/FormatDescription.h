/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace io {

struct FormatDescription {
	const char *name;
	const char *ext; //< just the file extension */

	inline core::String wildCard() const {
		static const core::String w("*.");
		return w + ext;
	}
};

extern core::String convertToFilePattern(const FormatDescription &desc);
extern core::String getWildcardsFromPattern(const core::String &pattern);

namespace format {
inline const FormatDescription* png() {
	static FormatDescription desc{"Image", "png"};
	return &desc;
}

inline const FormatDescription* lua() {
	static FormatDescription desc{"LUA script", "lua"};
	return &desc;
}

}

}
