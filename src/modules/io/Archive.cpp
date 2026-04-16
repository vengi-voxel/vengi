/**
 * @file
 */

#include "Archive.h"
#include "core/Path.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/Stream.h"

namespace io {

bool Archive::init(const core::String &path, io::SeekableReadStream *stream) {
	return true;
}

void Archive::shutdown() {
}

bool Archive::exists(const core::Path &file) const {
	return exists(file.toString());
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
