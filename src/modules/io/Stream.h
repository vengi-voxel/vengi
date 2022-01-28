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

namespace io {

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
	bool readFormat(const char *fmt, ...);
};

/**
 * @brief ReadStream with the option to jump back and forth in while reading
 */
class SeekableReadStream : public ReadStream {
public:
	virtual ~SeekableReadStream() {}
	/**
	 * @return -1 on error - otherwise the current offset in the stream
	 */
	virtual int64_t seek(int64_t position, int whence = SEEK_SET) = 0;
	/**
	 * @return The amount of bytes that are available in this stream
	 */
	virtual int64_t size() const = 0;
	/**
	 * @return The current position in the stream
	 */
	virtual int64_t pos() const = 0;

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
	bool readLine(int length, char *strbuff);

	int64_t remaining() const;
	bool empty() const;
};

inline int64_t SeekableReadStream::remaining() const {
	return size() - pos();
}

inline bool SeekableReadStream::empty() const {
	return size() == 0;
}

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

	bool writeBool(bool value);

	bool writeInt8(int8_t val);
	bool writeInt16(int16_t word);
	bool writeInt32(int32_t dword);
	bool writeInt64(int64_t dword);
	bool writeUInt8(uint8_t val);
	bool writeUInt16(uint16_t word);
	bool writeUInt32(uint32_t dword);
	bool writeUInt64(uint64_t dword);
	bool writeFloat(float value);

	bool writeInt16BE(int16_t word);
	bool writeInt32BE(int32_t dword);
	bool writeInt64BE(int64_t dword);
	bool writeUInt16BE(uint16_t word);
	bool writeUInt32BE(uint32_t dword);
	bool writeUInt64BE(uint64_t dword);
	bool writeFloatBE(float value);

	bool writeStringFormat(bool terminate, CORE_FORMAT_STRING const char *fmt, ...) CORE_PRINTF_VARARG_FUNC(3);
	bool writeString(const core::String &string, bool terminate = true);
	bool writeFormat(const char *fmt, ...);
};

/**
 * @brief WriteStream with the option to jump back and forth in while writing
 */
class SeekableWriteStream : public WriteStream {
public:
	virtual ~SeekableWriteStream() {}
	/**
	 * @return -1 on error - otherwise the current offset in the stream
	 */
	virtual int64_t seek(int64_t position, int whence = SEEK_SET) = 0;
	virtual int64_t size() const = 0;
	virtual int64_t pos() const = 0;
};

} // namespace io
