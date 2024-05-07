/**
 * @file
 */

#pragma once

#include "io/Archive.h"
#include "io/Stream.h"

namespace io {

/**
 * @ingroup IO
 */
class ZipArchive : public Archive {
private:
	void *_zip = nullptr;
	void reset();

public:
	ZipArchive();
	virtual ~ZipArchive();

	static bool validStream(io::SeekableReadStream &stream);
	SeekableReadStream* readStream(const core::String &filePath) override;
	SeekableWriteStream* writeStream(const core::String &filePath) override;

	bool init(const core::String &path, io::SeekableReadStream *stream) override;
	void shutdown() override;
};

ArchivePtr openZipArchive(io::SeekableReadStream *stream);

} // namespace io
