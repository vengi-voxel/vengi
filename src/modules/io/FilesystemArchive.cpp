/**
 * @file
 */

#include "FilesystemArchive.h"
#include "core/Log.h"
#include "core/SharedPtr.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"

namespace io {

FilesystemArchive::FilesystemArchive(const io::FilesystemPtr &filesytem, bool sysmode)
	: _filesytem(filesytem), _sysmode(sysmode) {
}

FilesystemArchive::~FilesystemArchive() {
	FilesystemArchive::shutdown();
}

bool FilesystemArchive::init(const core::String &path, io::SeekableReadStream *stream) {
	return add(path);
}

bool FilesystemArchive::add(const core::String &path, const core::String &filter, int depth) {
	if (path.empty()) {
		return false;
	}
	ArchiveFiles files;
	const bool ret = _filesytem->list(path, files, filter, depth);
	_files.append(files);
	return ret;
}

bool FilesystemArchive::exists(const core::String &path) const {
	if (_sysmode) {
		return _filesytem->sysExists(path);
	}
	return _filesytem->exists(path);
}

bool FilesystemArchive::exists(const core::Path &path) const {
	if (_sysmode) {
		return _filesytem->sysExists(path.toNativePath());
	}
	return _filesytem->exists(path.toString());
}

void FilesystemArchive::list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const {
	if (!_files.empty()) {
		Super::list(basePath, out, filter);
		return;
	}
	_filesytem->list(basePath, out, filter);
}

SeekableReadStream *FilesystemArchive::readStream(const core::String &filePath) {
	const io::FilePtr &file = _filesytem->open(filePath, _sysmode ? FileMode::SysRead : FileMode::Read);
	if (!file->validHandle()) {
		Log::error("Could not open file %s for reading: %s", file->name().c_str(), file->lastError().c_str());
		return nullptr;
	}
	io::FileStream *stream = new io::FileStream(file);
	core_assert(stream->valid());
	return stream;
}

SeekableWriteStream *FilesystemArchive::writeStream(const core::String &filePath) {
	const io::FilePtr &file = _filesytem->open(filePath, _sysmode ? FileMode::SysWrite : FileMode::Write);
	if (!file->validHandle()) {
		Log::error("Could not open file %s for writing: %s", file->name().c_str(), file->lastError().c_str());
		return nullptr;
	}
	io::FileStream *stream = new io::FileStream(file);
	core_assert(stream->valid());
	return stream;
}

ArchivePtr openFilesystemArchive(const io::FilesystemPtr &fs, const core::String &path, bool sysmode) {
	core::SharedPtr<FilesystemArchive> fa = core::make_shared<FilesystemArchive>(fs, sysmode);
	if (!path.empty() && fs->sysIsReadableDir(path)) {
		fa->init(path);
	}
	return fa;
}

} // namespace io
