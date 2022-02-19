/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/NonCopyable.h"
#include "core/String.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @defgroup IO IO
 * @{
 */

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

	bool readBool();
	int readInt8(int8_t &val);
	int readInt16(int16_t &val);
	int readInt32(int32_t &val);
	int readInt64(int64_t &val);
	int readUInt8(uint8_t &val);
	int readUInt16(uint16_t &val);
	int readUInt32(uint32_t &val);
	int readUInt64(uint64_t &val);
	int readFloat(float &val);

	int readInt8BE(int8_t &val);
	int readInt16BE(int16_t &val);
	int readInt32BE(int32_t &val);
	int readInt64BE(int64_t &val);
	int readUInt16BE(uint16_t &val);
	int readUInt32BE(uint32_t &val);
	int readUInt64BE(uint64_t &val);
	int readFloatBE(float &val);
	/**
	 * @brief Read a fixed-width string from a file. It may be null-terminated, but
	 * the position of the stream is still advanced by the given length
	 * @param[in] length The fixed length of the string in the file and the min length
	 * of the output buffer.
	 * @param[out] strbuff The output buffer
	 * @param[in] terminated If this is true, the read will stop on a 0 byte
	 */
	bool readString(int length, char *strbuff, bool terminated = false);
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
	 * @return The amount of bytes that are available in this stream
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
	 */
	int peekUInt32(uint32_t &val);
	int peekUInt16(uint16_t &val);
	int peekUInt8(uint8_t &val);
	/**
	 * @brief Reads from the buffer until newline characters were detected, a null byte was
	 * found or the end of the stream was reached.
	 * @param length The max length of the target buffer
	 * @param strbuff The target buffer
	 */
	bool readLine(int length, char *strbuff);

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
	return size() == 0;
}

/**
 * @ingroup IO
 */
class WriteStream : public core::NonCopyable {
public:
	virtual ~WriteStream() {}
	/**
	 * @return -1 on error - writte bytes on success
	 */
	virtual int write(const void *buf, size_t size) = 0;

	virtual bool flush() {
		return true;
	}

	bool writeBool(bool val);

	bool writeInt8(int8_t val);
	bool writeInt16(int16_t val);
	bool writeInt32(int32_t val);
	bool writeInt64(int64_t val);
	bool writeUInt8(uint8_t val);
	bool writeUInt16(uint16_t val);
	bool writeUInt32(uint32_t val);
	bool writeUInt64(uint64_t val);
	bool writeFloat(float val);

	bool writeInt16BE(int16_t val);
	bool writeInt32BE(int32_t val);
	bool writeInt64BE(int64_t val);
	bool writeUInt16BE(uint16_t val);
	bool writeUInt32BE(uint32_t val);
	bool writeUInt64BE(uint64_t val);
	bool writeFloatBE(float val);

	bool writeStringFormat(bool terminate, CORE_FORMAT_STRING const char *fmt, ...) CORE_PRINTF_VARARG_FUNC(3);
	/**
	 * @param terminate If this is @c true the extra null byte is written to the stream
	 * @return @c false if not everything was written
	 */
	bool writeString(const core::String &string, bool terminate = true);
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

} // namespace io


/**
 * @}
 */
