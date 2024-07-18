/**
 * @file
 */

#include "io/Stream.h"

namespace io {

class EndianStreamReadWrapper {
private:
	ReadStream &_stream;
	const bool _bigEndian;

public:
	EndianStreamReadWrapper(ReadStream &stream, bool bigEndian) : _stream(stream), _bigEndian(bigEndian) {
	}

	int64_t skipDelta(int64_t delta) {
		return _stream.skipDelta(delta);
	}

	bool readBool() {
		return _stream.readBool();
	}
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt8(int8_t &val) {
		return _stream.readInt8(val);
	}
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt16(int16_t &val) {
		if (_bigEndian) {
			return _stream.readInt16BE(val);
		}
		return _stream.readInt16(val);
	}
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt32(int32_t &val) {
		if (_bigEndian) {
			return _stream.readInt32BE(val);
		}
		return _stream.readInt32(val);
	}
	/**
	 * @return -1 on error - 0 on success
	 */
	int readInt64(int64_t &val) {
		if (_bigEndian) {
			return _stream.readInt64BE(val);
		}
		return _stream.readInt64(val);
	}
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt8(uint8_t &val) {
		return _stream.readUInt8(val);
	}
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt16(uint16_t &val) {
		if (_bigEndian) {
			return _stream.readUInt16BE(val);
		}
		return _stream.readUInt16(val);
	}
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt32(uint32_t &val) {
		if (_bigEndian) {
			return _stream.readUInt32BE(val);
		}
		return _stream.readUInt32(val);
	}
	/**
	 * @return -1 on error - 0 on success
	 */
	int readUInt64(uint64_t &val) {
		if (_bigEndian) {
			return _stream.readUInt64BE(val);
		}
		return _stream.readUInt64(val);
	}
	/**
	 * @return -1 on error - 0 on success
	 */
	int readFloat(float &val) {
		if (_bigEndian) {
			return _stream.readFloatBE(val);
		}
		return _stream.readFloat(val);
	}
	/**
	 * @return -1 on error - 0 on success
	 */
	int readDouble(double &val) {
		if (_bigEndian) {
			return _stream.readDoubleBE(val);
		}
		return _stream.readDouble(val);
	}
	bool readString(int length, char *strbuff, bool terminated = false) {
		return _stream.readString(length, strbuff, terminated);
	}
	bool readString(int length, core::String &strbuff, bool terminated = false) {
		return _stream.readString(length, strbuff, terminated);
	}
	bool readLine(core::String &str) {
		return _stream.readLine(str);
	}
	bool readPascalStringUInt8(core::String &str) {
		return _stream.readPascalStringUInt8(str);
	}
	bool readPascalStringUInt16(core::String &str) {
		if (_bigEndian) {
			return _stream.readPascalStringUInt16BE(str);
		}
		return _stream.readPascalStringUInt16LE(str);
	}

	bool readPascalStringUInt32(core::String &str) {
		if (_bigEndian) {
			return _stream.readPascalStringUInt32BE(str);
		}
		return _stream.readPascalStringUInt32LE(str);
	}
};

} // namespace io
