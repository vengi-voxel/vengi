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
	int64_t _pos;

public:
	StreamArchive(io::SeekableReadStream *stream) : _readStream(stream), _writeStream(nullptr) {
		_pos = stream->pos();
	}
	StreamArchive(io::SeekableWriteStream *stream) : _readStream(nullptr), _writeStream(stream) {
		_pos = stream->pos();
	}
	~StreamArchive() override = default;

	SeekableReadStream *readStream(const core::String &filePath) override {
		_readStream->seek(_pos);
		return new SeekableReadWriteStreamWrapper(_readStream);
	}
	SeekableWriteStream *writeStream(const core::String &filePath) override {
		_writeStream->seek(_pos);
		return new SeekableReadWriteStreamWrapper(_writeStream);
	}
};

} // namespace io
