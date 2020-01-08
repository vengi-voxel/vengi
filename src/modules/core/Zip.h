/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

namespace core {
namespace zip {

extern uint32_t compressBound(uint32_t in);
extern bool compress(const uint8_t *inputBuf, size_t inputBufSize,
		uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize = nullptr);
extern bool uncompress(const uint8_t *inputBuf, size_t inputBufSize,
		uint8_t* outputBuf, size_t outputBufSize, size_t* finalBufSize = nullptr);

}
}
