/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "io/Stream.h"

namespace io {
class FileStream;
class BufferedReadWriteStream;
class Archive;
typedef core::SharedPtr<Archive> ArchivePtr;
} // namespace io

namespace http {

/**
 * @brief Download from the given url and store it in the given file. If the file already exists, it will not get
 * downloaded again.
 *
 * @ingroup IO
 */
class HttpCacheStream : public io::SeekableReadStream {
private:
	io::SeekableReadStream *_readStream = nullptr;
	bool _newInCache = false;
	void write(const io::ArchivePtr &archive, const core::String &file, io::BufferedReadWriteStream &bufStream);

public:
	/**
	 * @param[in] file The path to the file stored in the archive
	 * @sa io::Filesystem::homePath()
	 */
	HttpCacheStream(const io::ArchivePtr &archive, const core::String &file, const core::String &url);
	virtual ~HttpCacheStream();

	bool valid() const;
	int read(void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
	int64_t size() const override;
	int64_t pos() const override;
	bool isNewInCache() const;
};

inline bool HttpCacheStream::isNewInCache() const {
	return _newInCache;
}

} // namespace http
