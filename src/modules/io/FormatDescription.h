/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <stdint.h>

namespace io {

struct FormatDescription {
	const char *name;
	const char *ext; /**< just the file extension */
	bool (*isA)(uint32_t magic);
	uint32_t flags;

	inline core::String wildCard() const {
		static const core::String w("*.");
		return w + ext;
	}
};

extern core::String convertToFilePattern(const FormatDescription &desc);
extern core::String getWildcardsFromPattern(const core::String &pattern);

namespace format {

inline const FormatDescription* png() {
	static FormatDescription desc[] = {
		{"Image", "png", nullptr, 0u},
		{nullptr, nullptr, nullptr, 0u}
	};
	return desc;
}

inline const FormatDescription* lua() {
	static FormatDescription desc[] = {
		{"LUA script", "lua", nullptr, 0},
		{nullptr, nullptr, nullptr, 0u}
	};
	return desc;
}

}

}
