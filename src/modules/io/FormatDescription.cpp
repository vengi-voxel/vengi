/**
 * @file
 */

#include "FormatDescription.h"
#include "core/StringUtil.h"

namespace io {

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
