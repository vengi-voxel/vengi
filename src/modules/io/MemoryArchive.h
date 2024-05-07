/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/collection/StringMap.h"
#include "io/Archive.h"

namespace io {

class BufferedReadWriteStream;

/**
 * Archive that stores files in memory.
 *
 * @note Only previously 'written' files get read back.
 *
 * @ingroup IO
 */
class MemoryArchive : public Archive {
private:
	core::StringMap<BufferedReadWriteStream *> _entries;

public:
	virtual ~MemoryArchive();
	bool init(const core::String &path, io::SeekableReadStream *stream) override;
	void shutdown() override;
	bool add(const core::String &name, const uint8_t *data, size_t size);
	bool remove(const core::String &name);
	SeekableReadStream *readStream(const core::String &filePath) override;
	SeekableWriteStream *writeStream(const core::String &filePath) override;
};

using MemoryArchivePtr = core::SharedPtr<MemoryArchive>;

inline MemoryArchivePtr openMemoryArchive() {
	return core::make_shared<MemoryArchive>();
}

} // namespace io
