/**
 * @file
 */

#pragma once

#include "core/collection/StringMap.h"

namespace io {
class SeekableReadStream;
}

namespace util {

using IniSectionMap = core::StringMap<core::String>;
using IniMap = core::StringMap<IniSectionMap>;

/**
 * @brief Parses the [section] key=value entries
 */
bool parseIniSection(io::SeekableReadStream &stream, IniSectionMap &values);
bool parseIni(io::SeekableReadStream &stream, IniMap &ini);
core::String getIniSectionValue(const IniSectionMap &values, const core::String &key, const core::String &defaultValue);

} // namespace util
