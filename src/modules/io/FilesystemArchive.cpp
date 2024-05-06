/**
 * @file
 */

#include "FilesystemArchive.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"

namespace io {

FilesystemArchive::FilesystemArchive(const io::FilesystemPtr &filesytem, bool fullPath)
	: _filesytem(filesytem), _fullPath(fullPath) {
}

FilesystemArchive::~FilesystemArchive() {
	FilesystemArchive::shutdown();
}

bool FilesystemArchive::init(const core::String &path, io::SeekableReadStream *stream) {
	return add(path);
}

bool FilesystemArchive::add(const core::String &path, const core::String &filter, int depth) {
	ArchiveFiles files;
	const bool ret = _filesytem->list(path, files, filter, depth);
	if (_fullPath) {
		for (FilesystemEntry &e : files) {
			e.fullPath = core::string::path(path, e.fullPath);
		}
	}
	_files.append(files);
	return ret;
}

io::FilePtr FilesystemArchive::open(const core::String &path, FileMode mode) const {
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
	Log::warn("Could not open %s", path.c_str());
	return {};
}

bool FilesystemArchive::exists(const core::String &filePath) {
	return open(filePath, FileMode::Read);
}

bool FilesystemArchive::load(const core::String &filePath, io::SeekableWriteStream &out) {
	const io::FilePtr &file = open(filePath, FileMode::Read);
	if (!file) {
		Log::error("Failed to load archive file: %s", filePath.c_str());
		return false;
	}
	io::FileStream stream(file);
	return out.write(stream);
}

SeekableReadStreamPtr FilesystemArchive::readStream(const core::String &filePath) {
	const core::SharedPtr<io::FileStream> &stream = core::make_shared<io::FileStream>(open(filePath, FileMode::Read));
	if (!stream->valid()) {
		return {};
	}
	return stream;
}

SeekableWriteStreamPtr FilesystemArchive::writeStream(const core::String &filePath) {
	const core::SharedPtr<io::FileStream> &stream = core::make_shared<io::FileStream>(open(filePath, FileMode::Write));
	if (!stream->valid()) {
		return {};
	}
	return stream;
}

} // namespace io
