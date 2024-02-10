/**
 * @file
 */

#include "Archive.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include "io/ZipArchive.h"

namespace io {

bool Archive::init(const core::String &path, io::SeekableReadStream *stream) {
	return true;
}

void Archive::shutdown() {
	_files.clear();
}

SeekableReadStreamPtr Archive::readStream(const core::String &filePath) {
	core::SharedPtr<BufferedReadWriteStream> stream = core::make_shared<BufferedReadWriteStream>();
	if (!load(filePath, *(stream.get()))) {
		return SeekableReadStreamPtr{};
	}
	stream->seek(0);
	return stream;
}

bool isSupportedArchive(const core::String &filename) {
	const core::String ext = core::string::extractExtension(filename);
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
	if (ext == "zip" || ext == "pk3" || (stream != nullptr && ZipArchive::validStream(*stream))) {
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
