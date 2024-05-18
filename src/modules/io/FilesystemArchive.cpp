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

FilesystemArchive::FilesystemArchive(const io::FilesystemPtr &filesytem, bool sysmode) : _filesytem(filesytem), _sysmode(sysmode) {
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

static FileMode convertMode(const core::String &path, FileMode mode) {
	if (core::string::isAbsolutePath(path)) {
		if (mode == FileMode::Read) {
			return FileMode::SysRead;
		}
		if (mode == FileMode::Write) {
			return FileMode::SysWrite;
		}
		core_assert_msg(false, "mode not supported: %d", (int)mode);
	}
	return mode;
}

io::FilePtr FilesystemArchive::open(const core::String &path, FileMode mode) const {
	Log::debug("searching for %s in %i files", path.c_str(), (int)_files.size());
	for (const auto &e : _files) {
		// TODO: implement case insensitive search
		if (core::string::endsWith(e.fullPath, path)) {
			const io::FilePtr &file = _filesytem->open(e.fullPath, mode);
			if (!file->validHandle()) {
				Log::debug("Could not open %s", e.fullPath.c_str());
				continue;
			}
			return file;
		}
		Log::trace("%s doesn't match %s", e.fullPath.c_str(), path.c_str());
	}
	if (_files.empty()) {
		const io::FilePtr &file = _filesytem->open(path, convertMode(path, mode));
		if (file->validHandle()) {
			return file;
		}
	}
	Log::warn("Could not open %s", path.c_str());
	return {};
}

bool FilesystemArchive::exists(const core::String &path) const {
	for (const auto &e : _files) {
		// TODO: implement case insensitive search
		if (core::string::endsWith(e.fullPath, path)) {
			return true;
		}
	}
	if (_files.empty()) {
		return _filesytem->exists(path);
	}
	return false;
}

void FilesystemArchive::list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const {
	if (!_files.empty()) {
		Super::list(basePath, out, filter);
		return;
	}
	_filesytem->list(basePath, out, filter);
}

SeekableReadStream *FilesystemArchive::readStream(const core::String &filePath) {
	io::FileStream *stream = new io::FileStream(open(filePath, _sysmode ? FileMode::SysRead : FileMode::Read));
	if (!stream->valid()) {
		delete stream;
		return nullptr;
	}
	return stream;
}

SeekableWriteStream *FilesystemArchive::writeStream(const core::String &filePath) {
	io::FileStream *stream = new io::FileStream(open(filePath, _sysmode ? FileMode::SysWrite : FileMode::Write));
	if (!stream->valid()) {
		delete stream;
		return nullptr;
	}
	return stream;
}

ArchivePtr openFilesystemArchive(const io::FilesystemPtr &fs, const core::String &path, bool sysmode) {
	core::SharedPtr<FilesystemArchive> fa = core::make_shared<FilesystemArchive>(fs, sysmode);
	if (!path.empty() && fs->isReadableDir(path)) {
		fa->init(path);
	}
	return fa;
}

} // namespace io
