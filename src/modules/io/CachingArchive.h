/**
 * @file
 */

#pragma once

#include "core/collection/DynamicStringMap.h"
#include "io/Archive.h"

namespace io {

/**
 * @brief Caches file lookups from registered search directories for fast case-insensitive
 * filename resolution.
 *
 * Each call to registerSearchDir() scans the given directory via the wrapped archive,
 * adding matching files to an internal name-to-path map (including immediate subdirectories).
 * findStream() resolves by relative path (e.g. dir/filename.ext) or by filename alone.
 *
 * @ingroup IO
 */
class CachingArchive {
private:
	ArchivePtr _archive;
	core::DynamicStringMap<core::String> _nameToPath;

	void registerSearchDirRecursive(const core::String &path, const core::String &filter,
									const core::String &cachePrefix);

public:
	CachingArchive(const ArchivePtr &archive);

	/**
	 * @brief Register a directory to scan and add matching files to the cache.
	 * @param path The directory path to scan (includes immediate subdirectories)
	 * @param filter File filter (e.g. "*.dat,*.ldr")
	 */
	void registerSearchDir(const core::String &path, const core::String &filter);

	/**
	 * @brief Find a readable stream by filename with case-insensitive lookup.
	 * @return Stream pointer (ownership transferred to caller) or nullptr
	 */
	SeekableReadStream *findStream(const core::String &filename);
	bool exists(const core::String &file) const;
	core::String fullPath(const core::String &file) const;
};

} // namespace io
