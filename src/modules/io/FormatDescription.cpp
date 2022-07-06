/**
 * @file
 */

#include "FormatDescription.h"
#include "core/String.h"
#include "core/StringUtil.h"

namespace io {

namespace format {

const FormatDescription* images() {
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
		{"", {}, nullptr, 0u}
	};
	return desc;
}

const FormatDescription* lua() {
	static FormatDescription desc[] = {
		{"LUA script", {"lua"}, nullptr, 0},
		{"", {}, nullptr, 0u}
	};
	return desc;
}

}

bool FormatDescription::matchesExtension(const core::String &fileExt) const {
	const core::String &lowerExt = fileExt.toLower();
	for (const core::String& ext : exts) {
		if (lowerExt == ext) {
			return true;
		}
	}
	return false;
}

bool FormatDescription::operator<(const FormatDescription &rhs) const {
	return name < rhs.name;
}

core::String FormatDescription::wildCard() const {
	core::String pattern;
	for (size_t i = 0; i < exts.size(); ++i) {
		if (i > 0) {
			pattern.append(",");
		}
		pattern += core::string::format("*.%s", exts[i].c_str());
	}
	return pattern;
}

bool isImage(const core::String& file) {
	const core::String& ext = core::string::extractExtension(file).toLower();
	for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
		if (desc->matchesExtension(ext)) {
			return true;
		}
	}
	return false;
}

core::String convertToFilePattern(const FormatDescription &desc) {
	core::String pattern;
	bool showName = false;
	if (!desc.name.empty()) {
		pattern += desc.name;
		if (!desc.exts.empty()) {
			pattern.append(" (");
			showName = true;
		}
	}
	for (size_t i = 0; i < desc.exts.size(); ++i) {
		if (i > 0) {
			pattern.append(",");
		}
		pattern += core::string::format("*.%s", desc.exts[i].c_str());
	}
	if (showName) {
		pattern.append(")");
	}
	return pattern;
}

core::String convertToAllFilePattern(const FormatDescription *desc) {
	core::String pattern;
	int j = 0;
	while (desc->valid()) {
		for (const core::String& ext : desc->exts) {
			if (j > 0) {
				pattern.append(",");
			}
			pattern += core::string::format("*.%s", ext.c_str());
			++j;
		}
		++desc;
	}
	return pattern;
}

core::String getWildcardsFromPattern(const core::String &pattern) {
	if (pattern.last() != ')') {
		return "";
	}
	const size_t i = pattern.find("(");
	if (i == core::String::npos) {
		return pattern;
	}
	// skip the two chars for ()
	const size_t len = pattern.size() - (i + 2);
	return pattern.substr(i + 1, len);
}

}
