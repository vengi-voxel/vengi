/**
 * @file
 */

#include "IniParser.h"
#include "core/StringUtil.h"
#include "io/Stream.h"

namespace util {

bool parseIniSection(io::SeekableReadStream &stream, core::StringMap<core::String> &values) {
	core::String line;
	while (stream.readLine(line)) {
		if (line.empty()) {
			return true;
		}
		if (core::string::startsWith(line, ";")) {
			continue;
		}
		auto i = line.find("=");
		if (i == core::String::npos) {
			return false;
		}
		const core::String &key = line.substr(0, i);
		const core::String &value = line.substr(i + 1);
		values.put(key, value);
	}
	return true;
}

} // namespace util
