/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/String.h"
#include "core/NonCopyable.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @defgroup IO IO
 * @{
 */

namespace core {
class String;
class UUID;
}

namespace io {

/**
 * @ingroup IO
 */
class ReadStream : public core::NonCopyable {
public:
	virtual ~ReadStream() {}
	/**
	 * @return -1 on error - read bytes on success
	 */
	virtual int read(void *dataPtr, size_t dataSize) = 0;
	virtual bool eos() const = 0;
	virtual void close() {}

	/**
	 * @brief Skips bytes
	 * @param delta the bytes to skip
	 * @return -1 on error
	 */
	int64_t skipDelta(int64_t delta);

	bool readBool();
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt8(int8_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt16(int16_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt32(int32_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt64(int64_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt8(uint8_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt16(uint16_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt32(uint32_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt64(uint64_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readFloat(float &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readDouble(double &val);

	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt8BE(int8_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt16BE(int16_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt32BE(int32_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt64BE(int64_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt16BE(uint16_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt32BE(uint32_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt64BE(uint64_t &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readFloatBE(float &val);
	/**
	 * @return -1 on error - 0 on success
	 */
	int readDoubleBE(double &val);
	int readUUID(core::UUID &uuid);
	/**
	 * @brief Read a fixed-width string from a file. It may be null-terminated, but
	 * the position of the stream is still advanced by the given length
	 * @param[in] length The fixed length of the string in the file and the min length
	 * of the output buffer.
	 * @param[out] strbuff The output buffer
	 * @param[in] terminated If this is true, the read will stop on a 0 byte
	 */
	bool readString(int length, char *strbuff, bool terminated = false);
	bool readString(int length, core::String &strbuff, bool terminated = false);
	/**
	 * @brief Allows you to read values by specifying a format string and pointers
	 * to the values to store them in.
	 * @code
	 * stream.readFormat("bsil", &valByte, &valShort, &valInt, &valLong)
	 * @endcode
	 *
	 * @param fmt Valid format identifiers are b for byte (uint8_t), s for short (uint16_t),
	 * i for int (uint32_t) and l for long (uint64_t).
	 * @return @c false if reading the values from the stream failed
	 * @sa WriteStream::writeFormat()
	 */
	bool readFormat(const char *fmt, ...);

	virtual bool readLine(core::String &str);

	bool readPascalStringUInt8(core::String &str);
	bool readPascalStringUInt16LE(core::String &str);
	bool readPascalStringUInt16BE(core::String &str);
	bool readPascalStringUInt32LE(core::String &str, uint32_t maxLength = (uint32_t)-1);
	bool readPascalStringUInt32BE(core::String &str, uint32_t maxLength = (uint32_t)-1);

	bool readUTF16BE(uint16_t characters, core::String &str);
};

/**
 * @brief ReadStream with the option to jump back and forth in while reading
 * @ingroup IO
 */
class SeekableReadStream : public ReadStream {
public:
	virtual ~SeekableReadStream() {}
	/**
	 * @param[in] position This is the number of bytes to offset
	 * @param[in] whence @c SEEK_SET offset is used as absolute position from the beginning of the stream.
	 * @c SEEK_CUR offset is taken as relative offset from the current position.
	 * @c SEEK_END offset is used relative to the end of the stream.
	 * @return -1 on error - otherwise the current offset in the stream
	 */
	virtual int64_t seek(int64_t position, int whence = SEEK_SET) = 0;
	/**
	 * @return The amount of bytes that are available in this stream or -1 on error
	 */
	virtual int64_t size() const = 0;
	/**
	 * @return The current position in the stream. Every read advances this position.
	 * @sa seek()
	 */
	virtual int64_t pos() const = 0;

	/**
	 * @brief Advances the position in the stream without reading the bytes.
	 * @param delta the bytes to skip
	 * @return -1 on error - otherwise the current offset in the stream
	 */
	int64_t skip(int64_t delta);

	bool eos() const override {
		return pos() >= size();
	}

	/**
	 * @note doesn't advance the stream position
	 * @return -1 on error - 0 on success
	 */
	int peekUInt32(uint32_t &val);
	/**
	 * @note doesn't advance the stream position
	 * @return -1 on error - 0 on success
	 */
	int peekInt32(int32_t &val);
	/**
	 * @note doesn't advance the stream position
	 * @return -1 on error - 0 on success
	 */
	int peekUInt16(uint16_t &val);
	/**
	 * @note doesn't advance the stream position
	 * @return -1 on error - 0 on success
	 */
	int peekUInt8(uint8_t &val);
	/**
	 * @brief Reads from the buffer until newline characters were detected, a null byte was
	 * found or the end of the stream was reached.
	 * @param length The max length of the target buffer
	 * @param strbuff The target buffer
	 */
	bool readLine(int length, char *strbuff);
	bool readLine(core::String &str) override;

	/**
	 * @return The amount of bytes left in the stream to read
	 * @sa size()
	 * @sa pos()
	 */
	int64_t remaining() const;
	bool empty() const;
};

inline int64_t SeekableReadStream::remaining() const {
	return size() - pos();
}

inline bool SeekableReadStream::empty() const {
	// even invalid streams (a size of -1) are empty
	return size() <= 0;
}

/**
 * @ingroup IO
 */
class WriteStream : public core::NonCopyable {
public:
	virtual ~WriteStream() {}
	/**
	 * @return -1 on error - written bytes on success
	 */
	virtual int write(const void *buf, size_t size) = 0;
	virtual void close() {}

	virtual bool flush() {
		return true;
	}

	bool writeStream(ReadStream &stream);

	bool writeBool(bool val);

	bool writeInt8(int8_t val);
	bool writeInt16(int16_t val);
	bool writeInt32(int32_t val);
	bool writeInt64(int64_t val);
	bool writeUInt8(uint8_t val);
	bool writeUInt16(uint16_t val);
	bool writeUInt32(uint32_t val);
	bool writeUInt64(uint64_t val);
	bool writeUUID(const core::UUID &uuid);
	bool writeFloat(float val);
	bool writeDouble(double val);

	bool writeInt16BE(int16_t val);
	bool writeInt32BE(int32_t val);
	bool writeInt64BE(int64_t val);
	bool writeUInt16BE(uint16_t val);
	bool writeUInt32BE(uint32_t val);
	bool writeUInt64BE(uint64_t val);
	bool writeFloatBE(float val);
	bool writeDoubleBE(double val);

	bool writeStringFormat(bool terminate, CORE_FORMAT_STRING const char *fmt, ...) CORE_PRINTF_VARARG_FUNC(3);
	/**
	 * @param terminate If this is @c true the extra null byte is written to the stream
	 * @return @c false if not everything was written
	 */
	bool writeString(const core::String &string, bool terminate);
	bool writeLine(const core::String &string, const char *lineEnding = "\n");

	/**
	 * @brief Allows you to write values by specifying a format string and the values to add to the stream
	 * @code
	 * stream.writeFormat("bsil", valByte, valShort, valInt, valLong)
	 * @endcode
	 *
	 * @param fmt Valid format identifiers are b for byte (uint8_t), s for short (uint16_t),
	 * i for int (uint32_t) and l for long (uint64_t).
	 * @return @c false if writing the values from the stream failed
	 * @sa ReadStream::readFormat()
	 */
	bool writeFormat(const char *fmt, ...);

	bool writeUTF16BE(const core::String &str);

	bool writePascalStringUInt8(const core::String &str);

	bool writePascalStringUInt32LE(const core::String &str);
	bool writePascalStringUInt32BE(const core::String &str);

	bool writePascalStringUInt16LE(const core::String &str);
	bool writePascalStringUInt16BE(const core::String &str);
};

/**
 * @brief WriteStream with the option to jump back and forth in while writing
 * @ingroup IO
 */
class SeekableWriteStream : public WriteStream {
public:
	virtual ~SeekableWriteStream() {}
	/**
	 * @param[in] position This is the number of bytes to offset
	 * @param[in] whence @c SEEK_SET offset is used as absolute position from the beginning of the stream.
	 * @c SEEK_CUR offset is taken as relative offset from the current position.
	 * @c SEEK_END offset is used relative to the end of the stream.
	 * @return -1 on error - otherwise the current offset in the stream
	 */
	virtual int64_t seek(int64_t position, int whence = SEEK_SET) = 0;
	/**
	 * @return The amount of bytes that were already written to the stream
	 */
	virtual int64_t size() const = 0;
	/**
	 * @return The current position in the stream. Every write advances this position.
	 * @sa seek()
	 */
	virtual int64_t pos() const = 0;
};

class SeekableReadWriteStream : public SeekableReadStream, public SeekableWriteStream {
private:
	using Super = SeekableReadStream;

public:
	virtual ~SeekableReadWriteStream() {
	}

	void close() override {
		SeekableReadStream::close();
		SeekableWriteStream::close();
	}
	virtual int64_t size() const override = 0;
	virtual int64_t pos() const override = 0;
	virtual int64_t seek(int64_t position, int whence = SEEK_SET) override = 0;
	virtual bool eos() const override = 0;
};

class NOPWriteStream : public io::SeekableWriteStream {
public:
	int write(const void *buf, size_t size) override {
		return (int)size;
	}
	int64_t seek(int64_t position, int whence = SEEK_SET) override {
		return 0;
	}
	int64_t size() const override {
		return 0;
	}
	int64_t pos() const override {
		return 0;
	}
};

// This class doesn't own the memory of the streams
class SeekableReadWriteStreamWrapper : public io::SeekableWriteStream, public io::SeekableReadStream {
private:
	io::SeekableWriteStream* _ws;
	io::SeekableReadStream* _rs;

public:
	SeekableReadWriteStreamWrapper(io::SeekableWriteStream* ws) : _ws(ws), _rs(nullptr) {
	}

	SeekableReadWriteStreamWrapper(io::SeekableReadStream* rs) : _ws(nullptr), _rs(rs) {
	}

	virtual ~SeekableReadWriteStreamWrapper() = default;

	int write(const void *buf, size_t size) override {
		if (_ws) {
			return _ws->write(buf, size);
		}
		return -1;
	}

	int read(void *dataPtr, size_t dataSize) override {
		if (_rs) {
			return _rs->read(dataPtr, dataSize);
		}
		return -1;
	}

	int64_t seek(int64_t position, int whence = SEEK_SET) override {
		if (_rs) {
			return _rs->seek(position, whence);
		}
		if (_ws) {
			return _ws->seek(position, whence);
		}
		return -1;
	}

	int64_t size() const override {
		if (_rs) {
			return _rs->size();
		}
		if (_ws) {
			return _ws->size();
		}
		return -1;
	}

	int64_t pos() const override {
		if (_rs) {
			return _rs->pos();
		}
		if (_ws) {
			return _ws->pos();
		}
		return -1;
	}
};

template<class SeekableStream>
class ScopedStreamPos {
private:
	SeekableStream *_stream;
	int64_t _pos;

public:
	ScopedStreamPos(SeekableStream *stream) : _stream(stream) {
		_pos = _stream->pos();
	}
	ScopedStreamPos(SeekableStream &stream) : _stream(&stream) {
		_pos = _stream->pos();
	}

	~ScopedStreamPos() {
		_stream->seek(_pos);
	}
};

} // namespace io


/**
 * @}
 */
