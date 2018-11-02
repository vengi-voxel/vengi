/**
 * @file
 */

#include "Zip.h"
#include "Log.h"

namespace core {

int Zip::printError(const char *context, int ret) {
	if (ret == Z_STREAM_ERROR) {
		Log::warn("Stream error while compressing input data: %s", context);
	} else if (ret == Z_BUF_ERROR) {
		Log::warn("Output buffer error - not enough space?: %s", context);
	} else if (ret == Z_STREAM_END) {
		Log::debug("Still pending input data left to compress: %s", context);
	} else if (ret == Z_NEED_DICT) {
		Log::debug("Need dict: %s", context);
	} else if (ret == Z_DATA_ERROR) {
		Log::warn("Data error: %s", context);
	} else if (ret == Z_MEM_ERROR) {
		Log::warn("Memory error: %s", context);
	}
	return ret;
}

bool Zip::uncompress(const uint8_t *inputBuf, size_t inputBufSize,
		uint8_t* outputBuf, size_t outputBufSize) {
	zalloc = Z_NULL;
	zfree = Z_NULL;
	opaque = Z_NULL;
	next_in = (z_const Bytef*)inputBuf;
	avail_in = (uInt)inputBufSize;
	next_out = (Bytef*)outputBuf;
	avail_out = (uInt)outputBufSize;
	int ret = printError("inflateInit2", inflateInit2(this, -MAX_WBITS));
	if (ret != Z_OK) {
		return false;
	}
	ret = printError("inflate", inflate(this, Z_SYNC_FLUSH));
	if (ret != Z_STREAM_END) {
		inflateEnd(this);
		return false;
	}
	return printError("inflateEnd", inflateEnd(this)) == Z_OK;
}

bool Zip::compress(const uint8_t *inputBuf, size_t inputBufSize,
		uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize, int compressionLevel) {
	zalloc = Z_NULL;
	zfree = Z_NULL;
	opaque = Z_NULL;
	next_in = (z_const Bytef*)inputBuf;
	avail_in = (uInt)inputBufSize;
	next_out = (Bytef*)outputBuf;
	avail_out = (uInt)outputBufSize;
	int ret = printError("deflateInit2", deflateInit2(this, compressionLevel, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY));
	if (ret != Z_OK) {
		return false;
	}
	ret = printError("deflate", deflate(this, Z_FINISH));
	if (finalBufSize != nullptr) {
		*finalBufSize = (size_t)total_out;
	}
	if (ret != Z_STREAM_END) {
		deflateEnd(this);
		return false;
	}
	return printError("deflateEnd", deflateEnd(this)) == Z_OK;
}

}
