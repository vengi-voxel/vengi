/**
 * @file
 */

#include "FilesystemArchive.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
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
	const core::String archiveFilePath = core::string::path(_path, filePath);
	io::FileStream stream(io::filesystem()->open(archiveFilePath));
	if (!stream.valid()) {
		Log::error("Failed to load archive file: %s", archiveFilePath.c_str());
		return false;
	}
	return out.write(stream);
}

} // namespace io
