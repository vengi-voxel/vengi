/**
 * @file
 */

#pragma once

#include "io/Archive.h"

namespace io {

/**
 * @brief Decorator archive that caches file listings from a wrapped archive.
 *
 * Provides O(1) exists() lookups and avoids repeated list() calls to the underlying
 * archive. The cache is populated lazily on first access and invalidated when
 * writeStream() or write() is called.
 *
 * @ingroup IO
 */
class CachingArchive : public Archive {
private:
	ArchivePtr _archive;
	mutable ArchiveFiles _cache;
	mutable bool _dirty = true;

	void fillCache() const;

public:
	CachingArchive(const ArchivePtr &archive);
	~CachingArchive() override;

	bool init(const core::String &path, io::SeekableReadStream *stream) override;
	void shutdown() override;

	bool exists(const core::String &file) const override;
	void list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const override;

	SeekableReadStream *readStream(const core::String &filePath) override;
	SeekableWriteStream *writeStream(const core::String &filePath) override;
	bool write(const core::String &filePath, io::ReadStream &stream) override;

	/**
	 * @brief Force the cache to be rebuilt on next access
	 */
	void invalidate();
};

ArchivePtr openCachingArchive(const ArchivePtr &archive);

} // namespace io
