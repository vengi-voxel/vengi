/**
 * @file
 */

#include "CachingArchive.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicStringMap.h"

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
	if (!core::string::startsWith(normalized, prefix)) {
		return false;
	}
	relative = normalized.substr(prefix.size());
	return true;
}

} // namespace priv

CachingArchive::CachingArchive(const ArchivePtr &archive) : _archive(archive) {
}

void CachingArchive::registerSearchDirRecursive(const core::String &path, const core::String &filter,
											  const core::String &cachePrefix) {
	const core::String prefix = priv::dirPrefix(path);
	ArchiveFiles files;
	_archive->list(path, files, filter);

	core::DynamicStringMap<bool> subdirs;
	int cached = 0;

	for (const FilesystemEntry &entry : files) {
		if (!entry.isFile()) {
			continue;
		}
		core::String relative;
		if (!priv::relativePath(path, entry.fullPath, relative)) {
			continue;
		}
		const size_t slash = relative.find("/");
		if (slash != core::String::npos) {
			if (slash > 0) {
				subdirs.put(relative.substr(0, slash), true);
			}
			continue;
		}

		const core::String lowerName = entry.name.toLower();
		if (!_nameToPath.hasKey(lowerName)) {
			_nameToPath.put(lowerName, entry.fullPath);
		}
		if (!cachePrefix.empty()) {
			const core::String relKey = core::string::path(cachePrefix, entry.name).toLower();
			if (!_nameToPath.hasKey(relKey)) {
				_nameToPath.put(relKey, entry.fullPath);
			}
		}
		++cached;
	}

	ArchiveFiles entries;
	_archive->list(path, entries, "");
	for (const FilesystemEntry &entry : entries) {
		if (entry.isDirectory()) {
			subdirs.put(entry.name, true);
		}
	}

	for (const auto &iter : subdirs) {
		const core::String subPath = core::string::path(path, iter->first);
		const core::String subPrefix =
			cachePrefix.empty() ? iter->first : core::string::path(cachePrefix, iter->first);
		registerSearchDirRecursive(subPath, filter, subPrefix);
	}

	Log::debug("Registered search dir '%s' with filter '%s' (%i files cached)", path.c_str(), filter.c_str(), cached);
}

void CachingArchive::registerSearchDir(const core::String &path, const core::String &filter) {
	registerSearchDirRecursive(path, filter, core::String::Empty);
}

static bool lookupCachedPath(const core::DynamicStringMap<core::String> &nameToPath, const core::String &filename,
							 core::String &fullPath) {
	const core::String sanitized = core::string::sanitizePath(filename);
	const core::String lower = sanitized.toLower();
	if (nameToPath.get(lower, fullPath)) {
		return true;
	}
	const core::String baseName = core::string::extractFilenameWithExtension(sanitized).toLower();
	if (baseName != lower && nameToPath.get(baseName, fullPath)) {
		return true;
	}
	return false;
}

bool CachingArchive::exists(const core::String &file) const {
	core::String fullPath;
	return lookupCachedPath(_nameToPath, file, fullPath);
}

core::String CachingArchive::fullPath(const core::String &file) const {
	core::String fullPath;
	if (lookupCachedPath(_nameToPath, file, fullPath)) {
		return fullPath;
	}
	return file;
}

SeekableReadStream *CachingArchive::findStream(const core::String &filename) {
	core::String fullPath;
	if (!lookupCachedPath(_nameToPath, filename, fullPath)) {
		return nullptr;
	}
	return _archive->readStream(fullPath);
}

} // namespace io
