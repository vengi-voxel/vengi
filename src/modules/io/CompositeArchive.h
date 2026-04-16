/**
 * @file
 */

#pragma once

#include "io/Archive.h"
#include "core/collection/DynamicArray.h"

namespace io {

/**
 * @brief An archive that delegates to multiple child archives in order.
 *
 * When reading a file, each child archive is tried in sequence until one succeeds.
 * File listings are merged from all child archives. This allows combining any archive
 * types (e.g. a ZipArchive with a FilesystemArchive for additional lookup paths).
 *
 * @ingroup IO
 */
class CompositeArchive : public Archive {
private:
	core::DynamicArray<ArchivePtr> _archives;

public:
	CompositeArchive() = default;
	~CompositeArchive() override;

	/**
	 * @brief Add a child archive to the composite. Archives added first have higher priority.
	 */
	void add(const ArchivePtr &archive);

	bool init(const core::String &path, io::SeekableReadStream *stream) override;
	void shutdown() override;

	bool exists(const core::String &file) const override;
	bool exists(const core::Path &file) const override;
	void list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const override;

	SeekableReadStream *readStream(const core::String &filePath) override;
	SeekableWriteStream *writeStream(const core::String &filePath) override;
};

} // namespace io
