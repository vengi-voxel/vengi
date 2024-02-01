/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "io/Stream.h"

namespace io {
class FileStream;
class BufferedReadWriteStream;
class Filesystem;
typedef core::SharedPtr<Filesystem> FilesystemPtr;
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
	const core::String _file;
	const core::String _url;
	io::FileStream *_fileStream = nullptr;
	bool _newInCache = false;

public:
	/**
	 * @param[in] file The path to the file stored in the virtual filesystem
	 * @sa io::Filesystem::homePath()
	 */
	HttpCacheStream(const io::FilesystemPtr &fs, const core::String &file, const core::String &url);
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
