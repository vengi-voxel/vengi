/**
 * @file
 */

#include "Stream.h"
#include "core/collection/Buffer.h"

namespace io {

/**
 * @note This buffer must be flushed
 */
class BufferedSeekableWriteStream : public SeekableWriteStream {
private:
	SeekableWriteStream &_stream;
	core::Buffer<uint8_t> _buffer;

public:
	/**
	 * @param[in] bufferedBytes The amount of bytes to buffer before the write is executed on the real buffer.
	 * @note The buffered bytes are 32 byte aligned
	 */
	BufferedSeekableWriteStream(SeekableWriteStream &stream, int bufferedBytes = 1 * 1024 * 1024) : _stream(stream) {
		_buffer.reserve(bufferedBytes);
	}
	virtual ~BufferedSeekableWriteStream() {
		BufferedSeekableWriteStream::flush();
	}

	inline size_t reservedBytes() const {
		return _buffer.capacity();
	}

	int write(const void *buf, size_t size) override {
		if (_buffer.capacity() < size) {
			flush();
			if (_stream.write(buf, size) != (int)size) {
				return -1;
			}
			return (int)size;
		}

		const size_t freeBytes = _buffer.capacity() - _buffer.size();
		const size_t remaining = freeBytes >= size ? 0 : freeBytes - size;
		if (remaining == 0) {
			_buffer.append((const uint8_t *)buf, size);
		} else {
			flush();
			_buffer.append((const uint8_t *)buf, size);
		}
		return (int)size;
	}

	bool flush() override {
		_stream.write(_buffer.data(), _buffer.size());
		_buffer.reset();
		return _stream.flush();
	}

	int64_t seek(int64_t position, int whence = SEEK_SET) override {
		flush();
		return _stream.seek(position, whence);
	}

	int64_t size() const override {
		return _stream.size() + (int64_t)_buffer.size();
	}

	int64_t pos() const override {
		return _stream.pos() + (int64_t)_buffer.size();
	}
};

} // namespace io
