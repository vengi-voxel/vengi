/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <stdint.h>

namespace io {

/**
 * @brief If the entry type is a link, the name is the symlink name, the fullPath is the target of the symlink.
 */
struct FilesystemEntry {
	core::String name;
	core::String fullPath;
	enum class Type : uint8_t {
		file,
		dir,
		link,
		unknown
	};
	Type type = Type::unknown;
	uint64_t size = 0u;		/**< size in bytes */
	uint64_t mtime = 0u;	/**< last modification time in millis */

	bool setExtension(const core::String &ext);

	inline bool isFile() const {
		if (type == Type::link) {
			return linkTargetType() == Type::file;
		}
		return type == Type::file;
	}

	inline bool isDirectory() const {
		if (type == Type::link) {
			return linkTargetType() == Type::dir;
		}
		return type == Type::dir;
	}

	inline bool isLink() const {
		return type == Type::link;
	}

	Type linkTargetType() const;
};

FilesystemEntry createFilesystemEntry(const core::String &filename);

}
