/**
 * @file
 */

#include "FilesystemArchive.h"
#include "core/Log.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"

namespace io {

FilesystemArchive::FilesystemArchive(const io::FilesystemPtr &filesystem, bool sysmode)
	: _filesystem(filesystem), _sysmode(sysmode) {
}

FilesystemArchive::~FilesystemArchive() {
}

bool FilesystemArchive::init(const core::String &path, io::SeekableReadStream *stream) {
	return true;
}

bool FilesystemArchive::exists(const core::String &path) const {
	const core::String normalized = core::string::sanitizePath(path);
	if (_sysmode) {
		return _filesystem->sysExists(normalized);
	}
	return _filesystem->exists(normalized);
}

bool FilesystemArchive::exists(const core::Path &path) const {
	return exists(path.toString());
}

void FilesystemArchive::list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const {
	_filesystem->list(basePath, out, filter);
}

SeekableReadStream *FilesystemArchive::readStream(const core::String &filePath) {
	const core::String normalized = core::string::sanitizePath(filePath);
	const io::FilePtr &file = _filesystem->open(normalized, _sysmode ? FileMode::SysRead : FileMode::Read);
	if (!file->validHandle()) {
		Log::error("Could not open file %s for reading: %s", file->name().c_str(), file->lastError().c_str());
		return nullptr;
	}
	io::FileStream *stream = new io::FileStream(file);
	core_assert(stream->valid());
	return stream;
}

SeekableWriteStream *FilesystemArchive::writeStream(const core::String &filePath) {
	const core::String normalized = core::string::sanitizePath(filePath);
	const io::FilePtr &file = _filesystem->open(normalized, _sysmode ? FileMode::SysWrite : FileMode::Write);
	if (!file->validHandle()) {
		Log::error("Could not open file %s for writing: %s", file->name().c_str(), file->lastError().c_str());
		return nullptr;
	}
	io::FileStream *stream = new io::FileStream(file);
	core_assert(stream->valid());
	return stream;
}

ArchivePtr openFilesystemArchive(const io::FilesystemPtr &fs, const core::String &path, bool sysmode) {
	return core::make_shared<FilesystemArchive>(fs, sysmode);
}

} // namespace io
