/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <stdint.h>

namespace io {

struct FormatDescription {
	const char *name = nullptr;				/**< the name of the format */
	const char *exts[8];					/**< the file extension - nullptr terminated list */
	bool (*isA)(uint32_t magic) = nullptr;	/**< function to check whether a magic byte matches for the format description */
	uint32_t flags = 0u;					/**< flags for user defines properties */

	bool operator<(const FormatDescription &rhs) const;
	core::String wildCard() const;
	bool matchesExtension(const core::String &fileExt) const;
};

extern core::String convertToAllFilePattern(const FormatDescription *desc);
extern core::String convertToFilePattern(const FormatDescription &desc);
extern core::String getWildcardsFromPattern(const core::String &pattern);
extern bool isImage(const core::String& file);

namespace format {

inline const FormatDescription* images() {
	static FormatDescription desc[] = {
		{"Portable Network Graphics", {"png"}, nullptr, 0u},
		{"JPEG", {"jpeg", "jpg"}, nullptr, 0u},
		{"Targa image file", {"tga"}, nullptr, 0u},
		{"Bitmap", {"bmp"}, nullptr, 0u},
		{"Photoshop", {"psd"}, nullptr, 0u},
		{"Graphics Interchange Format", {"gif"}, nullptr, 0u},
		{"Radiance rgbE", {"hdr"}, nullptr, 0u},
		{"Softimage PIC", {"pic"}, nullptr, 0u},
		{"Portable Anymap", {"pnm"}, nullptr, 0u},
		{nullptr, {}, nullptr, 0u}
	};
	return desc;
}

inline const FormatDescription* lua() {
	static FormatDescription desc[] = {
		{"LUA script", {"lua"}, nullptr, 0},
		{nullptr, {}, nullptr, 0u}
	};
	return desc;
}

}

}
