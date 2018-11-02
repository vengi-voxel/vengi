/**
 * @file
 */

#pragma once

#include <SDL.h>
#include <zlib.h>

namespace core {

/**
 * @brief Wrapper around zlib z_stream
 */
class Zip: public z_stream {
private:
	static int printError(const char *context, int ret);
public:
	bool uncompress(const uint8_t *inputBuf, size_t inputBufSize,
			uint8_t* outputBuf, size_t outputBufSize);

	/**
	 * @brief Compresses the given input buffer and store the result in the given output buffer
	 * @param[in] inputBuf The buffer to compress
	 * @param[in] inputBufSize The size of the input buffer
	 * @param[out] outputBuf The buffer to store the compressed data in
	 * @param[in] outputBufSize The size of the output buffer
	 * @param[out] finalBufSize The size that was really used in the output buffer. Can be null.
	 * @return @c true if the compression was successful, @c false otherwise.
	 */
	bool compress(const uint8_t *inputBuf, size_t inputBufSize,
			uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize = nullptr, int compressionLevel = Z_DEFAULT_COMPRESSION);
};

namespace zip {

inline bool compress(const uint8_t *inputBuf, size_t inputBufSize,
			uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize = nullptr, int compressionLevel = Z_DEFAULT_COMPRESSION) {
	Zip z;
	return z.compress(inputBuf, inputBufSize, outputBuf, outputBufSize, finalBufSize, compressionLevel);
}

inline bool uncompress(const uint8_t *inputBuf, size_t inputBufSize,
			uint8_t* outputBuf, size_t outputBufSize) {
	Zip z;
	return z.uncompress(inputBuf, inputBufSize, outputBuf, outputBufSize);
}

}

}
