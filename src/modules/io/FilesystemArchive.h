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
	bool _sysmode;

	io::FilePtr open(const core::String &path, FileMode mode) const;

public:
	FilesystemArchive(const io::FilesystemPtr &filesytem, bool sysmode = true);
	virtual ~FilesystemArchive();
	bool init(const core::String &path, io::SeekableReadStream *stream = nullptr) override;
	bool add(const core::String &path, const core::String &filter = "", int depth = 0);
	bool exists(const core::String &file) const override;
	void list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const override;

	SeekableReadStream *readStream(const core::String &filePath) override;
	SeekableWriteStream *writeStream(const core::String &filePath) override;
};

/**
 * @param[in] sysmode Specifies the use of the @c FileMode values when opening files for reading or writing.
 */
ArchivePtr openFilesystemArchive(const io::FilesystemPtr &fs, const core::String &path = "", bool sysmode = true);

} // namespace io
