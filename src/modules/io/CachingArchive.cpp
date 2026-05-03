/**
 * @file
 */

#include "CachingArchive.h"
#include "core/Log.h"
#include "core/StringUtil.h"

namespace io {

CachingArchive::CachingArchive(const ArchivePtr &archive) : _archive(archive) {
}

void CachingArchive::registerSearchDir(const core::String &path, const core::String &filter) {
	ArchiveFiles files;
	_archive->list(path, files, filter);
	for (const FilesystemEntry &entry : files) {
		if (!entry.isFile()) {
			continue;
		}
		const core::String lowerName = entry.name.toLower();
		if (!_nameToPath.hasKey(lowerName)) {
			_nameToPath.put(lowerName, entry.fullPath);
		}
	}
	Log::debug("Registered search dir '%s' with filter '%s' (%i files cached)", path.c_str(), filter.c_str(),
			   (int)files.size());
}

bool CachingArchive::exists(const core::String &file) const {
	const core::String name = core::string::extractFilenameWithExtension(file);
	return _nameToPath.hasKey(name.toLower());
}

core::String CachingArchive::fullPath(const core::String &file) const {
	const core::String name = core::string::extractFilenameWithExtension(file);
	core::String fullPath;
	if (_nameToPath.get(name.toLower(), fullPath)) {
		return fullPath;
	}
	return file;
}

SeekableReadStream *CachingArchive::findStream(const core::String &filename) {
	const core::String sanitized = core::string::sanitizePath(filename);
	const core::String name = core::string::extractFilenameWithExtension(sanitized);
	const core::String lowerName = name.toLower();

	core::String fullPath;
	if (_nameToPath.get(lowerName, fullPath)) {
		return _archive->readStream(fullPath);
	}

	return nullptr;
}

} // namespace io
