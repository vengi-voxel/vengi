/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <stdint.h>

namespace io {

struct FormatDescription {
	const char *name = nullptr;				/**< the name of the format */
	const char *ext = nullptr;				/**< the file extension */
	bool (*isA)(uint32_t magic) = nullptr;	/**< function to check whether a magic byte matches for the format description */
	uint32_t flags = 0u;					/**< flags for user defines properties */

	inline core::String wildCard() const {
		static const core::String w("*.");
		return w + ext;
	}
};

extern core::String convertToFilePattern(const FormatDescription &desc);
extern core::String getWildcardsFromPattern(const core::String &pattern);

namespace format {

inline const FormatDescription* images() {
	static FormatDescription desc[] = {
		{"Portable Network Graphics", "png", nullptr, 0u},
		{"JPEG", "jpeg", nullptr, 0u},
		{"JPEG", "jpg", nullptr, 0u},
		{"Targa image file", "tga", nullptr, 0u},
		{"Bitmap", "bmp", nullptr, 0u},
		{"Photoshop", "psd", nullptr, 0u},
		{"Graphics Interchange Format", "gif", nullptr, 0u},
		{"Radiance rgbE", "hdr", nullptr, 0u},
		{"Softimage PIC", "pic", nullptr, 0u},
		{"Portable Anymap", "pnm", nullptr, 0u},
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
