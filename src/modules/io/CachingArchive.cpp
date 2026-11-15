/**
 * @file
 */

#include "CachingArchive.h"
#include "core/Log.h"
#include "core/StringUtil.h"

namespace io {

namespace priv {

static core::String dirPrefix(const core::String &path) {
	core::String norm = core::string::sanitizePath(path);
	if (!norm.empty() && norm.last() != '/') {
		norm.append("/");
	}
	return norm;
}

static bool relativePath(const core::String &dirPath, const core::String &fullPath, core::String &relative) {
	const core::String prefix = dirPrefix(dirPath);
	const core::String normalized = core::string::sanitizePath(fullPath);
	if (prefix.empty()) {
		// If the prefix is empty (e.g. dirPath was "./" or ""), use the filename only
		relative = core::string::extractFilenameWithExtension(normalized);
		return true;
	}
	if (core::string::startsWith(normalized, prefix)) {
		relative = normalized.substr(prefix.size());
		return true;
	}
	// e.g. FilesystemArchive resolves relative search dirs against registered paths and returns
	// absolute fullPath (normalized) values. Match the search directory as a path-component suffix.
	const core::String needle = core::String("/") + prefix;
	const size_t pos = normalized.rfind(needle.c_str());
	if (pos != core::String::npos) {
		relative = normalized.substr(pos + needle.size());
		return true;
	}
	return false;
}

static bool matchesFilter(const core::String &name, const core::String &filter) {
	return filter.empty() || core::string::fileMatchesMultiple(name.c_str(), filter.c_str());
}

} // namespace priv

CachingArchive::CachingArchive(const ArchivePtr &archive) : _archive(archive) {
}

void CachingArchive::registerSearchDir(const core::String &path, const core::String &filter) {
	SearchRoot root;
	root.path = path;
	root.filter = filter;
	_roots.push_back(core::move(root));
	Log::debug("Registered search dir '%s' with filter '%s' (lazy)", path.c_str(), filter.c_str());
}

bool CachingArchive::lookupCachedPath(const core::String &filename, core::String &fullPath) const {
	const core::String sanitized = core::string::sanitizePath(filename);
	const core::String lower = sanitized.toLower();
	if (_nameToPath.get(lower, fullPath)) {
		return true;
	}
	const core::String baseName = core::string::extractFilenameWithExtension(sanitized).toLower();
	if (baseName != lower && _nameToPath.get(baseName, fullPath)) {
		return true;
	}
	return false;
}

bool CachingArchive::cacheEntry(const core::String &searchPath, const core::String &cachePrefix,
								const FilesystemEntry &entry) const {
	if (!entry.isFile()) {
		return false;
	}
	core::String relative;
	if (!priv::relativePath(searchPath, entry.fullPath, relative)) {
		if (entry.name.empty()) {
			return false;
		}
		relative = entry.name;
	}
	const size_t slash = relative.find("/");
	if (slash != core::String::npos) {
		return false;
	}

	bool added = false;
	const core::String lowerName = entry.name.toLower();
	if (!_nameToPath.hasKey(lowerName)) {
		_nameToPath.put(lowerName, entry.fullPath);
		added = true;
	}
	if (!cachePrefix.empty()) {
		const core::String relKey = core::string::path(cachePrefix, entry.name).toLower();
		if (!_nameToPath.hasKey(relKey)) {
			_nameToPath.put(relKey, entry.fullPath);
			added = true;
		}
	}
	return added;
}

void CachingArchive::listDirIntoCache(SearchRoot &root, const core::String &relDir) const {
	const core::String listKey = relDir.toLower();
	if (root.listedDirs.hasKey(listKey)) {
		return;
	}
	root.listedDirs.put(listKey, true);

	const core::String searchPath =
		relDir.empty() ? root.path : core::string::path(root.path, relDir);
	ArchiveFiles files;
	_archive->list(searchPath, files, root.filter);
	int cached = 0;
	for (const FilesystemEntry &entry : files) {
		if (cacheEntry(searchPath, relDir, entry)) {
			++cached;
		}
	}
	Log::debug("Listed search dir '%s' filter '%s' (%i files cached)", searchPath.c_str(), root.filter.c_str(),
			   cached);
}

bool CachingArchive::resolvePath(const core::String &filename, core::String &fullPath) const {
	if (lookupCachedPath(filename, fullPath)) {
		return true;
	}

	const core::String sanitized = core::string::sanitizePath(filename);
	const core::String baseName = core::string::extractFilenameWithExtension(sanitized);

	for (SearchRoot &root : _roots) {
		if (!priv::matchesFilter(baseName, root.filter)) {
			continue;
		}

		// Path relative to this search root (strip root prefix if the lookup includes it).
		core::String relUnderRoot = sanitized;
		const core::String prefix = priv::dirPrefix(root.path);
		if (!prefix.empty() && core::string::startsWith(relUnderRoot, prefix)) {
			relUnderRoot = relUnderRoot.substr(prefix.size());
		}

		const core::String candidateRel = core::string::path(root.path, relUnderRoot);
		if (_archive->exists(candidateRel)) {
			const core::String lower = sanitized.toLower();
			if (!_nameToPath.hasKey(lower)) {
				_nameToPath.put(lower, candidateRel);
			}
			if (baseName.toLower() != lower && !_nameToPath.hasKey(baseName.toLower())) {
				_nameToPath.put(baseName.toLower(), candidateRel);
			}
			fullPath = candidateRel;
			return true;
		}
		if (relUnderRoot != baseName) {
			const core::String candidateBase = core::string::path(root.path, baseName);
			if (_archive->exists(candidateBase)) {
				const core::String lowerBase = baseName.toLower();
				if (!_nameToPath.hasKey(lowerBase)) {
					_nameToPath.put(lowerBase, candidateBase);
				}
				fullPath = candidateBase;
				return true;
			}
		}

		// Case-insensitive / absolute-fullPath archives: list only the needed directory.
		const size_t slash = relUnderRoot.find("/");
		const core::String parentDir =
			slash == core::String::npos ? core::String::Empty : relUnderRoot.substr(0, slash);
		listDirIntoCache(root, parentDir);
		if (lookupCachedPath(filename, fullPath) || lookupCachedPath(relUnderRoot, fullPath) ||
			lookupCachedPath(baseName, fullPath)) {
			return true;
		}
	}
	return false;
}

bool CachingArchive::exists(const core::String &file) const {
	core::String resolved;
	return resolvePath(file, resolved);
}

core::String CachingArchive::fullPath(const core::String &file) const {
	core::String resolved;
	if (resolvePath(file, resolved)) {
		return resolved;
	}
	return file;
}

SeekableReadStream *CachingArchive::findStream(const core::String &filename) {
	core::String resolved;
	if (!resolvePath(filename, resolved)) {
		return nullptr;
	}
	return _archive->readStream(resolved);
}

} // namespace io
