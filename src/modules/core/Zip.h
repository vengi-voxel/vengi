/**
 * @file
 */

#pragma once

#include <zlib.h>

namespace core {

/**
 * @brief Wrapper around zlib z_stream
 */
class Zip: public z_stream {
public:
	Zip(bool inflate = true, int compressionLevel = Z_DEFAULT_COMPRESSION) :
			_inflate(inflate), _compressionLevel(compressionLevel) {
		this->zalloc = Z_NULL;
		this->zfree = Z_NULL;
		this->opaque = Z_NULL;
		this->avail_in = 0;
		this->next_in = Z_NULL;
		this->avail_out = 0;
		this->next_out = Z_NULL;
		init();
	}

	~Zip() {
		shutdown();
	}

	inline bool initialized() const {
		return _initialized;
	}

	bool shutdown() {
		if (!_initialized) {
			return false;
		}
		_initialized = false;
		if (_inflate) {
			inflateEnd(this);
		} else {
			deflateEnd(this);
		}
		return true;
	}

	bool uncompress(const uint8_t *inputBuf, size_t inputBufSize,
			uint8_t* outputBuf, size_t outputBufSize) {
		next_in = (z_const Bytef*)inputBuf;
		avail_in = (uInt)inputBufSize;
		next_out = (Bytef*)outputBuf;
		avail_out = (uInt)outputBufSize;
		const int ret = inflate(this, Z_NO_FLUSH);
		if (ret != Z_OK && ret != Z_STREAM_END) {
			return false;
		}
		return true;
	}

	bool compress(const uint8_t *inputBuf, size_t inputBufSize,
			uint8_t* outputBuf, size_t outputBufSize) {
		// TODO:
		next_out = (Bytef*)outputBuf;
		avail_out = (uInt)outputBufSize;
		const int ret = deflate(this, Z_FINISH);
		return ret == Z_OK;
	}

	bool init() {
		if (_initialized) {
			return true;
		}
		int ret;
		if (_inflate) {
			// 15 window bits, and the +32 tells zlib to detect if using gzip or zlib
			ret = inflateInit2(this, 15 + 32);
		} else {
			ret = deflateInit2(this, _compressionLevel, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
		}
		_initialized = ret == Z_OK;
		return _initialized;
	}
private:
	bool _inflate;
	bool _initialized = false;
	int _compressionLevel;
};

}
