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
	/**
	 * @param[in] filePath The relative filePath to the path the archive was initialized with
	 */
	bool load(const core::String &filePath, io::SeekableWriteStream &out) override;
};

} // namespace io
