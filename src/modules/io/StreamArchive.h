/**
 * @file
 */

#include "io/Archive.h"
#include "io/Stream.h"

namespace io {

/**
 * @ingroup IO
 */
class StreamArchive : public Archive {
private:
	io::SeekableReadStream *_readStream;
	io::SeekableWriteStream *_writeStream;

public:
	StreamArchive(io::SeekableReadStream *stream) : _readStream(stream) {
	}
	StreamArchive(io::SeekableWriteStream *stream) : _writeStream(stream) {
	}
	~StreamArchive() override = default;
	SeekableReadStream *readStream(const core::String &filePath) override {
		return new SeekableReadWriteStreamWrapper(_readStream);
	}
	SeekableWriteStream *writeStream(const core::String &filePath) override {
		return new SeekableReadWriteStreamWrapper(_writeStream);
	}
};

} // namespace io
