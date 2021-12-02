/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/String.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

namespace io {

class ReadStream {
public:
	virtual ~ReadStream() {}
	/**
	 * @return -1 on error - 0 on success
	 */
	virtual int read(void *dataPtr, size_t dataSize) = 0;
	virtual bool eos() const = 0;

	bool readBool();
	int readByte(uint8_t &val);
	int readShort(uint16_t &val);
	int readInt(uint32_t &val);
	int readLong(uint64_t &val);
	int readFloat(float &val);
	int readShortBE(uint16_t &val);
	int readIntBE(uint32_t &val);
	int readLongBE(uint64_t &val);
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

class SeekableReadStream : public ReadStream {
public:
	virtual ~SeekableReadStream() {}
	/**
	 * @return -1 on error - otherwise the current offset in the stream
	 */
	virtual int64_t seek(int64_t position, int whence = SEEK_SET) = 0;

	int64_t skip(int64_t delta);

	bool eos() const override {
		return pos() >= size();
	}

	int peekInt(uint32_t &val);
	int peekShort(uint16_t &val);
	int peekByte(uint8_t &val);
	bool readLine(int length, char *strbuff);

	int64_t remaining() const;
	bool empty() const;
	virtual int64_t size() const = 0;
	virtual int64_t pos() const = 0;
};

inline int64_t SeekableReadStream::remaining() const {
	return size() - pos();
}

inline bool SeekableReadStream::empty() const {
	return size() == 0;
}

class WriteStream {
public:
	virtual ~WriteStream() {}
	/**
	 * @return -1 on error - 0 on success
	 */
	virtual int write(const void *buf, size_t size) = 0;

	bool writeBool(bool value);
	bool writeByte(uint8_t val);

	bool writeShort(uint16_t word);
	bool writeInt(uint32_t dword);
	bool writeLong(uint64_t dword);
	bool writeFloat(float value);

	bool writeShortBE(uint16_t word);
	bool writeIntBE(uint32_t dword);
	bool writeLongBE(uint64_t dword);
	bool writeFloatBE(float value);

	bool writeStringFormat(bool terminate, CORE_FORMAT_STRING const char *fmt, ...) CORE_PRINTF_VARARG_FUNC(3);
	bool writeString(const core::String &string, bool terminate = true);
	bool writeFormat(const char *fmt, ...);
};

} // namespace io
