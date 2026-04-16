/**
 * @file
 */

#include "Archive.h"
#include "core/Algorithm.h"
#include "core/Path.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/Stream.h"

namespace io {

bool Archive::init(const core::String &path, io::SeekableReadStream *stream) {
	return true;
}

void Archive::shutdown() {
	_files.clear();
}

bool Archive::exists(const core::Path &file) const {
	return exists(file.toString());
}

bool Archive::exists(const core::String &file) const {
	const core::String normalized = core::string::sanitizePath(file);
	return core::find_if(_files.begin(), _files.end(), [&](const auto &e1) { return e1.fullPath == normalized; }) !=
		   _files.end();
}

void Archive::list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const {
	const core::String normalizedBase = core::string::sanitizePath(basePath);
	for (const auto &entry : _files) {
		if (!normalizedBase.empty() && !core::string::startsWith(entry.fullPath, normalizedBase)) {
			continue;
		}
		if (core::string::fileMatchesMultiple(entry.name.c_str(), filter.c_str())) {
			out.push_back(entry);
		}
	}
}

void Archive::list(const core::String &filter, ArchiveFiles &out) const {
	list("", out, filter);
}

SeekableWriteStream *Archive::writeStream(const core::String &filePath) {
	return nullptr;
}

bool Archive::write(const core::String &filePath, io::ReadStream &stream) {
	core::ScopedPtr<io::SeekableWriteStream> wstream(writeStream(filePath));
	if (!wstream) {
		return false;
	}
	return wstream->writeStream(stream);
}

bool isZipArchive(const core::String &filename) {
	const core::String &ext = core::string::extractExtension(filename);
	return ext == "zip" || ext == "pk3";
}

} // namespace io
