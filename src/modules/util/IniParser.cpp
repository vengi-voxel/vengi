/**
 * @file
 */

#include "IniParser.h"
#include "core/StringUtil.h"
#include "io/Stream.h"

namespace util {

bool parseIniSection(io::SeekableReadStream &stream, IniSectionMap &values) {
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

bool parseIni(io::SeekableReadStream &stream, IniMap &ini) {
	core::String line;
	while (stream.readLine(line)) {
		if (line.empty()) {
			continue;
		}
		if (core::string::startsWith(line, "[") && core::string::endsWith(line, "]")) {
			const core::String &section = line.substr(1, line.size() - 2);
			IniSectionMap values;
			if (!parseIniSection(stream, values)) {
				return false;
			}
			ini.put(section, values);
		}
	}
	return true;
}

} // namespace util
