/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace core {

/**
 * Represents a path in a file system.
 *
 * The internal representation of the path is always normalized to use forward slashes - if you need the native path,
 * use @c toNativePath()
 */
class Path {
private:
	core::String _path;

public:
	Path() = default;
	Path(const core::String &path);
	Path(core::String &&path);

	/**
	 * Returns the directory part of the path.
	 */
	Path dirname() const;

	/**
	 * Returns the file name part of the path.
	 */
	Path basename() const;

	/**
	 * Returns the extension part of the path.
	 */
	core::String extension() const;

	/**
	 * Returns the path without the extension part.
	 */
	Path removeExtension() const;

	/**
	 * Returns the path with the extension part replaced.
	 *
	 * @param newExtension The new extension to replace the old extension with.
	 * @return The path with the extension part replaced.
	 */
	Path replaceExtension(const core::String &newExtension) const;

	char separator() const;
	core::String toNativePath() const;

	core::DynamicArray<core::String> components() const;

	bool isRelativePath() const;
	bool isAbsolutePath() const;
	bool isRootPath() const;

	char driveLetter() const;

	Path append(const core::String &component) const;
	Path append(const core::Path &component) const;

	Path &operator+=(const core::String &other);
	Path &operator+=(const core::Path &other);

	bool operator==(const core::Path &other) const;
	bool operator!=(const core::Path &other) const;

	bool operator==(const core::String &other) const;
	bool operator!=(const core::String &other) const;

	inline const char *c_str() const {
		return _path.c_str();
	}

	inline const core::String &str() const {
		return _path;
	}

	inline bool operator<(const Path &x) const {
		return _path < x._path;
	}

	inline bool empty() const {
		return _path.empty();
	}
};

Path operator+(const Path &lhs, const core::Path &rhs);
Path operator+(const Path &lhs, const core::String &rhs);

} // namespace core
