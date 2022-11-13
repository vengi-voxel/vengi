/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <stdint.h>

namespace io {

struct FilesystemEntry {
	core::String name;
	enum class Type : uint8_t {
		file,
		dir,
		link,
		unknown
	};
	Type type = Type::unknown;
	uint64_t size = 0u;		/**< size in bytes */
	uint64_t mtime = 0u;	/**< last modification time in millis */
};

}
