/**
 * @file
 */

#include "io/Archive.h"
#include "io/Stream.h"

namespace io {

/**
 * @brief This is a simple archive implementation that wraps a single stream. It does not support multiple files, but
 * can be used to treat a stream as an archive for use with other code that expects an archive interface.
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

inline ArchivePtr openStreamArchive(io::SeekableReadStream *stream) {
	return core::make_shared<StreamArchive>(stream);
}

inline ArchivePtr openStreamArchive(io::SeekableWriteStream *stream) {
	return core::make_shared<StreamArchive>(stream);
}

} // namespace io
