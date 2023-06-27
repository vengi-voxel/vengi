/**
 * @file
 */

#pragma once

#include "core/collection/StringMap.h"

namespace io {
class SeekableReadStream;
}

namespace util {

/**
 * @brief Parses the [section] key=value entries
 */
bool parseIniSection(io::SeekableReadStream &stream, core::StringMap<core::String> &values);

}
