#pragma once

#include "core/collection/DynamicArray.h"
#include "core/miniz.h"
#include "io/Filesystem.h"
#include "io/Stream.h"

namespace io {

using ZipArchiveFiles = core::DynamicArray<FilesystemEntry>;

class ZipArchive {
private:
	mz_zip_archive *_zip = nullptr;
	ZipArchiveFiles _files;

public:
	ZipArchive();
	~ZipArchive();

	bool open(io::SeekableReadStream *stream);
	bool load(const core::String &file, io::SeekableWriteStream &out);
	void close();

	const ZipArchiveFiles &files() const;
};

inline const ZipArchiveFiles &ZipArchive::files() const {
	return _files;
}

} // namespace io
