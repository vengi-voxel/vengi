/**
 * @file
 */

#include "Zip.h"
#include "Log.h"
#include "Assert.h"
extern "C" {
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES 1
#include "miniz.h"
}

namespace core {
namespace zip {

bool uncompress(const uint8_t *inputBuf, size_t inputBufSize,
		uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize) {
	core_assert_msg(outputBufSize > 0, "Expected to get a outputBufSize > 0 - but got %i", (int)outputBufSize);
	core_assert_msg(inputBufSize > 0, "Expected to get a inputBufSize > 0 - but got %i", (int)inputBufSize);
	mz_ulong destLen = outputBufSize;
	int ret = ::mz_uncompress((unsigned char*)outputBuf, &destLen, (const unsigned char*) inputBuf, (mz_ulong)inputBufSize);
	if (ret == MZ_OK) {
		if (finalBufSize != nullptr) {
			*finalBufSize = (size_t)destLen;
		}
		return true;
	}

	if (ret == MZ_MEM_ERROR) {
		Log::error("Failed to uncompress input buffer of size %i into output buffer of size %i - there was not enough memory",
				(int)inputBufSize, (int)outputBufSize);
	}
	if (ret == MZ_BUF_ERROR) {
		Log::error("Failed to uncompress input buffer of size %i into output buffer of size %i - there was not enough room in the output buffer",
				(int)inputBufSize, (int)outputBufSize);
	}
	if (ret == MZ_DATA_ERROR) {
		Log::error("Failed to uncompress input buffer of size %i into output buffer of size %i - the input data was corrupted",
				(int)inputBufSize, (int)outputBufSize);
	}
	return false;
}

uint32_t compressBound(uint32_t in) {
	core_assert_msg(in > 0, "Expected to get a size > 0 - but got %i", (int)in);
	return (uint32_t)::mz_compressBound((mz_ulong)in);
}

bool compress(const uint8_t *inputBuf, size_t inputBufSize,
		uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize) {
	core_assert_msg(outputBufSize > 0, "Expected to get a outputBufSize > 0 - but got %i", (int)outputBufSize);
	core_assert_msg(inputBufSize > 0, "Expected to get a inputBufSize > 0 - but got %i", (int)inputBufSize);
	mz_ulong destLen = outputBufSize;
	int ret = ::mz_compress((unsigned char*)outputBuf, &destLen, (const unsigned char*) inputBuf, (mz_ulong)inputBufSize);
	if (ret == MZ_OK) {
		if (finalBufSize != nullptr) {
			*finalBufSize = (size_t)destLen;
		}
		return true;
	}
	if (ret == MZ_MEM_ERROR) {
		Log::error("Failed to compress input buffer of size %i into output buffer of size %i - there was not enough memory",
				(int)inputBufSize, (int)outputBufSize);
	}
	if (ret == MZ_BUF_ERROR) {
		Log::error("Failed to compress input buffer of size %i into output buffer of size %i - there was not enough room in the output buffer",
				(int)inputBufSize, (int)outputBufSize);
	}
	return false;
}

}
}
