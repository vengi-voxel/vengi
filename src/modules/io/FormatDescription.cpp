/**
 * @file
 */

#include "FormatDescription.h"
#include "core/StringUtil.h"

namespace io {

bool isImage(const core::String& file) {
	const core::String& ext = core::string::extractExtension(file);
	for (const io::FormatDescription *desc = io::format::images(); desc->ext != nullptr; ++desc) {
		if (ext == desc->ext) {
			return true;
		}
	}
	return false;
}


core::String convertToFilePattern(const FormatDescription &desc) {
	if (!desc.name || desc.name[0] == '\0') {
		return core::string::format("*.%s", desc.ext);
	}
	return core::string::format("%s (*.%s)", desc.name, desc.ext);
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
