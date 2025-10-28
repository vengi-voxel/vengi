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
	bool flush();
public:
	ZipArchive();
	virtual ~ZipArchive();

	bool isWrite() const;
	static bool validStream(io::SeekableReadStream &stream);
	SeekableReadStream* readStream(const core::String &filePath) override;
	SeekableWriteStream* writeStream(const core::String &filePath) override;

	bool init(const core::String &path, io::SeekableReadStream *stream) override;
	bool init(io::SeekableWriteStream *stream);
	void shutdown() override;
};

ArchivePtr openZipArchive(io::SeekableReadStream *stream);

} // namespace io
