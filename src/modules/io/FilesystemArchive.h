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
protected:
	io::FilesystemPtr _filesytem;

	io::FilePtr open(const core::String &path, FileMode mode) const;
public:
	FilesystemArchive(const io::FilesystemPtr &filesytem);
	virtual ~FilesystemArchive();
	bool init(const core::String &path, io::SeekableReadStream *stream) override;
	bool add(const core::String &path, const core::String &filter = "", int depth = 0);
	bool exists(const core::String &file) const override;
	bool load(const core::String &filePath, io::SeekableWriteStream &out) override;
	SeekableReadStreamPtr readStream(const core::String &filePath) override;
	SeekableWriteStreamPtr writeStream(const core::String &filePath) override;
};

} // namespace io
