/**
 * @file
 */

#include "Archive.h"
#include "core/Algorithm.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"

namespace io {

bool Archive::init(const core::String &path, io::SeekableReadStream *stream) {
	return true;
}

void Archive::shutdown() {
	_files.clear();
}

bool Archive::exists(const core::String &file) const {
	return core::find_if(_files.begin(), _files.end(), [&](const auto &e1) { return e1.fullPath == file; }) !=
		   _files.end();
}

SeekableReadStreamPtr Archive::readStream(const core::String &filePath) {
	const core::SharedPtr<BufferedReadWriteStream> &stream = core::make_shared<BufferedReadWriteStream>();
	if (!load(filePath, *(stream.get()))) {
		return SeekableReadStreamPtr{};
	}
	stream->seek(0);
	return stream;
}

SeekableWriteStreamPtr Archive::writeStream(const core::String &filePath) {
	return SeekableWriteStreamPtr{};
}

bool isSupportedArchive(const core::String &filename) {
	const core::String &ext = core::string::extractExtension(filename);
	return ext == "zip" || ext == "pk3";
}

ArchivePtr openArchive(const io::FilesystemPtr &fs, const core::String &path, io::SeekableReadStream *stream) {
	if (fs->isReadableDir(path)) {
		auto archive = core::make_shared<FilesystemArchive>(fs);
		if (!archive->init(path, stream)) {
			return ArchivePtr{};
		}
		return archive;
	}
	const core::String ext = core::string::extractExtension(path);
	if (ext == "zip" || ext == "pk3" || ext == "thing" || (stream != nullptr && ZipArchive::validStream(*stream))) {
		auto archive = core::make_shared<ZipArchive>();
		if (!archive->init(path, stream)) {
			return ArchivePtr{};
		}
		return archive;
	}
	const core::String &directory = core::string::extractPath(path);
	auto archive = core::make_shared<FilesystemArchive>(fs);
	if (!archive->init(directory, stream)) {
		return ArchivePtr{};
	}
	return archive;
}

} // namespace io
