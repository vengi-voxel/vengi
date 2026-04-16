/**
 * @file
 */

#include "io/Archive.h"
#include "io/Stream.h"

namespace io {

/**
 * @brief Archive adapter that wraps a single seekable stream.
 *
 * This does not support multiple files - @c exists() always returns false and
 * @c list() is a no-op. Every call to @c readStream() or @c writeStream()
 * seeks back to the position the underlying stream had at construction time,
 * so callers always read/write from that same starting offset. This is
 * intentional: the archive does not own the stream and cannot know whether
 * the caller has advanced it between calls.
 *
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

	bool exists(const core::String &) const override {
		return false;
	}
	void list(const core::String &, ArchiveFiles &, const core::String &) const override {
	}

	// Always resets to the construction-time position so the same data can
	// be re-read by successive callers (e.g. format probing followed by loading).
	SeekableReadStream *readStream(const core::String &filePath) override {
		if (_readStream == nullptr) {
			return nullptr;
		}
		_readStream->seek(_pos);
		return new SeekableReadWriteStreamWrapper(_readStream);
	}
	// Same reset semantics as readStream - see class documentation.
	SeekableWriteStream *writeStream(const core::String &filePath) override {
		if (_writeStream == nullptr) {
			return nullptr;
		}
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
