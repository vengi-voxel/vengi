/**
 * @file
 */

#include "FormatDescription.h"
#include "core/StringUtil.h"

namespace io {

bool FormatDescription::matchesExtension(const core::String &fileExt) const {
	const core::String &lowerExt = fileExt.toLower();
	for (int i = 0; i < lengthof(exts); ++i) {
		if (!exts[i]) {
			break;
		}
		if (lowerExt == exts[i]) {
			return true;
		}
	}
	return false;
}

bool isImage(const core::String& file) {
	const core::String& ext = core::string::extractExtension(file).toLower();
	for (const io::FormatDescription *desc = io::format::images(); desc->name != nullptr; ++desc) {
		if (desc->matchesExtension(ext)) {
			return true;
		}
	}
	return false;
}

core::String convertToFilePattern(const FormatDescription &desc) {
	core::String pattern;
	bool showName = false;
	if (desc.name && desc.name[0] != '\0') {
		pattern += desc.name;
		pattern.append(" (");
		showName = true;
	}
	for (int i = 0; i < lengthof(desc.exts); ++i) {
		if (!desc.exts[i]) {
			break;
		}
		if (i > 0) {
			pattern.append(",");
		}
		pattern += core::string::format("*.%s", desc.exts[i]);
	}
	if (showName) {
		pattern.append(")");
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
