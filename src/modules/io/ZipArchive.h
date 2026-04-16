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
	ArchiveFiles _files;
	void reset();
	bool flush();
public:
	ZipArchive();
	virtual ~ZipArchive();

	bool isWrite() const;
	static bool validStream(io::SeekableReadStream &stream);
	bool exists(const core::String &file) const override;
	void list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const override;
	SeekableReadStream* readStream(const core::String &filePath) override;
	SeekableWriteStream* writeStream(const core::String &filePath) override;

	bool init(const core::String &path, io::SeekableReadStream *stream) override;
	bool init(io::SeekableWriteStream *stream);
	void shutdown() override;
};

ArchivePtr openZipArchive(io::SeekableReadStream *stream);

} // namespace io
