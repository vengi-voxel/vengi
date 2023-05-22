/**
 * @file
 */

#include "FilesystemArchive.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"

namespace io {

FilesystemArchive::~FilesystemArchive() {
	FilesystemArchive::shutdown();
}

bool FilesystemArchive::init(const core::String &path, io::SeekableReadStream *stream) {
	_path = path;
	return io::filesystem()->list(path, _files);
}

bool FilesystemArchive::load(const core::String &filePath, io::SeekableWriteStream &out) {
	// TODO: use the file list to implement case insensitive search
	const core::String &archiveFilePath = core::string::path(_path, filePath);
	io::FileStream stream(io::filesystem()->open(archiveFilePath));
	if (!stream.valid()) {
		Log::error("Failed to load archive file: %s", archiveFilePath.c_str());
		return false;
	}
	return out.write(stream);
}

SeekableReadStreamPtr FilesystemArchive::readStream(const core::String &filePath) {
	const core::String &archiveFilePath = core::string::path(_path, filePath);
	const io::FilePtr &file = io::filesystem()->open(archiveFilePath);
	if (!file->validHandle()) {
		return {};
	}
	const core::SharedPtr<io::FileStream> &stream = core::make_shared<io::FileStream>(file);
	if (!stream->valid()) {
		return {};
	}
	return stream;
}

} // namespace io
