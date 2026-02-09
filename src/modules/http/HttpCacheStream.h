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

	/**
	 * @brief Check cache and download if necessary (GET request)
	 */
	void initGet(const io::ArchivePtr &archive, const core::String &file, const core::String &url);

	/**
	 * @brief Check cache and download if necessary (POST request)
	 */
	void initPost(const io::ArchivePtr &archive, const core::String &file, const core::String &url,
				  const core::String &postBody, const core::String &contentType);

public:
	/**
	 * @brief Constructor for GET requests
	 * @param[in] file The path to the file stored in the archive
	 * @sa io::Filesystem::homePath()
	 */
	HttpCacheStream(const io::ArchivePtr &archive, const core::String &file, const core::String &url);

	/**
	 * @brief Constructor for POST requests
	 * @param[in] file The path to the file stored in the archive
	 * @param[in] postBody The POST request body
	 * @param[in] contentType The Content-Type header value (e.g., "application/x-www-form-urlencoded")
	 */
	HttpCacheStream(const io::ArchivePtr &archive, const core::String &file, const core::String &url,
					const core::String &postBody, const core::String &contentType = "application/x-www-form-urlencoded");

	virtual ~HttpCacheStream();

	void close() override;
	bool valid() const;
	int read(void *dataPtr, size_t dataSize) override;
	int64_t seek(int64_t position, int whence = SEEK_SET) override;
	int64_t size() const override;
	int64_t pos() const override;
	bool isNewInCache() const;

	/**
	 * @brief Convenience method for GET requests
	 */
	static core::String string(const io::ArchivePtr &archive, const core::String &file, const core::String &url);

	/**
	 * @brief Convenience method for POST requests
	 */
	static core::String stringPost(const io::ArchivePtr &archive, const core::String &file, const core::String &url,
								   const core::String &postBody,
								   const core::String &contentType = "application/x-www-form-urlencoded");
};

inline bool HttpCacheStream::isNewInCache() const {
	return _newInCache;
}

} // namespace http
