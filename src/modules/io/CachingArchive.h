/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicStringMap.h"
#include "io/Archive.h"

namespace io {

/**
 * @brief Caches file lookups from registered search directories for fast case-insensitive
 * filename resolution.
 *
 * registerSearchDir() records a search root. Lookups first try direct archive->exists()
 * under each root (cheap for large libraries), then fall back to listing only the needed
 * directory for case-insensitive / absolute-fullPath archives. Successful resolutions are
 * cached; the whole tree is never indexed eagerly.
 *
 * findStream() resolves by relative path (e.g. dir/filename.ext) or by filename alone.
 * First registered root wins on basename collisions.
 *
 * @ingroup IO
 */
class CachingArchive {
private:
	struct SearchRoot {
		core::String path;
		core::String filter;
		core::DynamicStringMap<bool> listedDirs;
	};

	ArchivePtr _archive;
	mutable core::DynamicArray<SearchRoot> _roots;
	mutable core::DynamicStringMap<core::String> _nameToPath;

	bool lookupCachedPath(const core::String &filename, core::String &fullPath) const;
	bool cacheEntry(const core::String &searchPath, const core::String &cachePrefix,
					const FilesystemEntry &entry) const;
	void listDirIntoCache(SearchRoot &root, const core::String &relDir) const;
	bool resolvePath(const core::String &filename, core::String &fullPath) const;

public:
	CachingArchive(const ArchivePtr &archive);

	/**
	 * @brief Register a directory for filename lookups (lazy; no eager scan).
	 * @param path The directory path to search (includes subdirectories on demand)
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
