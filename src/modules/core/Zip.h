/**
 * @file
 */

#pragma once

#include <SDL.h>

namespace core {
namespace zip {

extern bool compress(const uint8_t *inputBuf, size_t inputBufSize,
		uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize = nullptr);
extern bool uncompress(const uint8_t *inputBuf, size_t inputBufSize,
		uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize = nullptr);

}
}
