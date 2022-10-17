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
		unknown
	};
	Type type;
	uint64_t size;	/**< size in bytes */
	uint64_t mtime;	/**< last modification time in millis */
};

}
