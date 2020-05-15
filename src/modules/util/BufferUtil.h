/**
 * @file
 */

#pragma once

#include <string.h>
#include <stdint.h>

namespace util {

/**
 * @brief Compresses the given indices to a smaller index type
 *
 * This function evaluates the max value of the given collection and tries to compress
 * the index buffer according to that value. If you e.g. given an index buffer with index
 * type having a size of 4 bytes, but your max index is 16, you will get a newly compressed
 * buffer where the indices are aligned to be used as 1 byte indices.
 *
 * @param[in] in The buffer with the indices
 * @param[in] inSize The input buffer size
 * @param[in] inIndexSize Size of one index in the buffer
 * @param[out] bytesPerIndex The compressed bytes per index
 * @param[out] buf The buffer with the compressed indices
 * @param[in] bufSize The target buffer size for the compressed indices
 */
extern void indexCompress(const uint8_t *in, size_t inSize, size_t inIndexSize, size_t &bytesPerIndex, uint8_t *buf, size_t bufSize);

/**
 * @brief Compresses the given indices to a smaller index type
 *
 * This function evaluates the max value of the given collection and tries to compress
 * the index buffer according to that value. If you e.g. given an index buffer with index
 * type having a size of 4 bytes, but your max index is 16, you will get a newly compressed
 * buffer where the indices are aligned to be used as 1 byte indices.
 *
 * @param[in] in The buffer with the indices
 * @param[in] inSize The input buffer size
 * @param[out] bytesPerIndex The compressed bytes per index
 * @param[out] buf The buffer with the compressed indices
 * @param[in] bufSize The target buffer size for the compressed indices
 */
template<class SRC, class TGT>
void indexCompress(const SRC *in, size_t inSize, size_t &bytesPerIndex, TGT *buf, size_t bufSize) {
	indexCompress((const uint8_t*)in, inSize, sizeof(SRC), bytesPerIndex, (uint8_t*)buf, bufSize);
}

}
