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
	explicit Path(const core::String &path);
	Path(const core::String &path1, const core::String &path2);
	explicit Path(core::String &&path);

	/**
	 * Returns the directory part of the path.
	 */
	[[nodiscard]] Path dirname() const;

	/**
	 * Returns the file name part of the path.
	 */
	[[nodiscard]] Path basename() const;

	[[nodiscard]] Path popFront() const;
	[[nodiscard]] Path popBack() const;

	[[nodiscard]] core::String lexicallyNormal() const;

	/**
	 * Returns the extension part of the path.
	 */
	[[nodiscard]] core::String extension() const;

	/**
	 * Returns the path without the extension part.
	 */
	[[nodiscard]] Path removeExtension() const;

	/**
	 * Returns the path with the extension part replaced.
	 *
	 * @param newExtension The new extension to replace the old extension with.
	 * @return The path with the extension part replaced.
	 */
	[[nodiscard]] Path replaceExtension(const core::String &newExtension) const;

	[[nodiscard]] char separator() const;
	[[nodiscard]] core::String toNativePath() const;
	[[nodiscard]] core::String toString() const;

	[[nodiscard]] core::DynamicArray<core::String> components() const;

	bool isRelativePath() const;
	bool isAbsolutePath() const;
	bool isRootPath() const;
	bool hasParentDirectory() const;

	char driveLetter() const;

	[[nodiscard]] Path append(const core::String &component) const;
	[[nodiscard]] Path append(const core::Path &component) const;

	Path &operator+=(const core::String &other);
	Path &operator+=(const core::Path &other);

	bool operator==(const core::Path &other) const;
	bool operator!=(const core::Path &other) const;

	bool operator==(const core::String &other) const;
	bool operator!=(const core::String &other) const;

	inline bool valid() const {
		return !_path.empty();
	}

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

inline core::String Path::toString() const {
	return _path;
}

} // namespace core
