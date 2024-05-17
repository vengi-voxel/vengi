/**
 * @file
 */

#pragma once

#include "io/Archive.h"
#include "io/File.h"
#include "io/Filesystem.h"

namespace io {

/**
 * @ingroup IO
 */
class FilesystemArchive : public Archive {
private:
	using Super = Archive;

protected:
	io::FilesystemPtr _filesytem;

	io::FilePtr open(const core::String &path, FileMode mode) const;

public:
	FilesystemArchive(const io::FilesystemPtr &filesytem);
	virtual ~FilesystemArchive();
	bool init(const core::String &path, io::SeekableReadStream *stream = nullptr) override;
	bool add(const core::String &path, const core::String &filter = "", int depth = 0);
	bool exists(const core::String &file) const override;
	void list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const override;

	SeekableReadStream *readStream(const core::String &filePath) override;
	SeekableWriteStream *writeStream(const core::String &filePath) override;
};

ArchivePtr openFilesystemArchive(const io::FilesystemPtr &fs, const core::String &path = "");

} // namespace io
