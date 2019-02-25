/**
 * @file
 */

#include "Zip.h"
#include "Log.h"
#include <zlib.h>

namespace core {
namespace zip {

bool uncompress(const uint8_t *inputBuf, size_t inputBufSize,
		uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize) {
	uLongf destLen = outputBufSize;
	if (::uncompress((Bytef*)outputBuf, &destLen, (const Bytef*) inputBuf, (uLong)inputBufSize) == Z_OK) {
		if (finalBufSize != nullptr) {
			*finalBufSize = (size_t)destLen;
		}
		return true;
	}
	return false;
}

bool compress(const uint8_t *inputBuf, size_t inputBufSize,
		uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize) {
	uLongf destLen = outputBufSize;
	if (::compress((Bytef*)outputBuf, &destLen, (const Bytef*) inputBuf, (uLong)inputBufSize) == Z_OK) {
		if (finalBufSize != nullptr) {
			*finalBufSize = (size_t)destLen;
		}
		return true;
	}
	return false;
}

}
}
