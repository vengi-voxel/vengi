/**
 * @file
 */

#include "Stream.h"
#include "core/collection/Buffer.h"

namespace io {

/**
 * @note This buffer must be flushed
 */
class BufferedWriteStream : public WriteStream {
private:
	WriteStream &_stream;
	core::Buffer<uint8_t> _buffer;

public:
	/**
	 * @param[in] bufferedBytes The amount of bytes to buffer before the write is executed on the real buffer.
	 * @note The buffered bytes are 32 byte aligned
	 */
	BufferedWriteStream(WriteStream &stream, int bufferedBytes = 1 * 1024 * 1024) : _stream(stream) {
		_buffer.reserve(bufferedBytes);
	}
	virtual ~BufferedWriteStream() {
		BufferedWriteStream::flush();
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
};

} // namespace io
