/**
 * @file
 */

#pragma once

#include "io/Archive.h"

namespace io {

class FilesystemArchive : public Archive {
protected:
	core::String _path;
public:
	virtual ~FilesystemArchive();
	bool init(const core::String &path, io::SeekableReadStream *stream) override;
	bool load(const core::String &filePath, io::SeekableWriteStream &out) override;
	SeekableReadStreamPtr readStream(const core::String &filePath) override;
};

} // namespace io
