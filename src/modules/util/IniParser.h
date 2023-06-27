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

} // namespace util
