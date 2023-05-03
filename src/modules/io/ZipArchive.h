/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/Stream.h"

namespace io {

class ZipArchive : public Archive {
private:
	void *_zip = nullptr;
	void reset();

public:
	ZipArchive();
	virtual ~ZipArchive();

	static bool validStream(io::SeekableReadStream &stream);

	bool init(const core::String &path, io::SeekableReadStream *stream) override;
	bool load(const core::String &file, io::SeekableWriteStream &out) override;
	void shutdown() override;
};

} // namespace io
