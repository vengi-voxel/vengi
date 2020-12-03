/**
 * @file
 */

#include <SDL_rwops.h>
#include <SDL_stdinc.h>

namespace core {

class MemoryStreamReadOnly {
private:
	const uint8_t *_buf;
	const uint32_t _size;
	uint32_t _pos = 0;

	/**
	 * @return A value of @c 0 indicates no error
	 * @note This does not handle the endianness
	 */
	template <class Ret>
	int peek(Ret &val) const {
		const size_t bufSize = sizeof(Ret);
		if (remaining() < (int64_t)bufSize) {
			return -1;
		}
		SDL_memcpy((void *)&val, _buf + _pos, bufSize);
		return 0;
	}

public:
	MemoryStreamReadOnly(const uint8_t *buf, uint32_t size) : _buf(buf), _size(size) {
	}
	~MemoryStreamReadOnly() {}

	template <class Ret>
	inline int read(Ret &val) {
		const int retVal = peek<Ret>(val);
		if (retVal == 0) {
			_pos += sizeof(val);
		}
		return retVal;
	}

	/**
	 * @return -1 on error
	 */
	int seek(int64_t position);

	inline int64_t remaining() const {
		return _size - _pos;
	}

	int readBuf(uint8_t *buf, size_t bufSize);

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
	bool readLine(int length, char *strbuff);
	bool readFormat(const char *fmt, ...);

	int peekInt(uint32_t &val) const;
	int peekShort(uint16_t &val) const;
	int peekIntBE(uint32_t &val) const;
	int peekShortBE(uint16_t &val) const;
	int peekByte(uint8_t &val) const;

	inline bool empty() const {
		return _size == 0;
	}

	inline bool eos() const {
		return _pos >= _size;
	}

	bool skip(int32_t delta) {
		if (_pos + delta >= _size) {
			_pos = _size;
			return false;
		}
		_pos += delta;
		return true;
	}

	inline uint32_t size() const {
		return _size;
	}

	inline uint32_t pos() const {
		return _pos;
	}
};

}
