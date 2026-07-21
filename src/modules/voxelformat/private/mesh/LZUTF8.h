/**
 * @file
 * @brief Minimal LZUTF8 StorageBinaryString support for Blockbench .bbmodel files
 *
 * Spec: <lz> + LZUTF8.compress(json, { outputEncoding: 'StorageBinaryString' })
 * See https://github.com/rotemdan/lzutf8.js
 */

#pragma once

#include "core/String.h"
#include "core/collection/Buffer.h"
#include <stdint.h>

namespace voxelformat {
namespace lzutf8 {

/**
 * @brief Decode StorageBinaryString (UTF-16 code units) to compressed bytes
 */
bool decodeStorageBinaryString(const uint16_t *chars, size_t charCount, core::Buffer<uint8_t> &out);

/**
 * @brief Encode compressed bytes to StorageBinaryString code units
 */
bool encodeStorageBinaryString(const uint8_t *bytes, size_t byteCount, core::Buffer<uint16_t> &out);

/**
 * @brief Decompress an LZUTF8 byte stream to UTF-8 output
 */
bool decompress(const uint8_t *input, size_t inputSize, core::Buffer<uint8_t> &out);

/**
 * @brief If @p raw starts with "<lz>", decompress StorageBinaryString payload to JSON text.
 * Otherwise returns @p raw with optional BOM stripped. Also strips JSON comments as a fallback path.
 */
core::String decodeBBModelText(const core::String &raw);

/**
 * @brief Strip line and block comments outside of JSON strings (Blockbench autoParseJSON fallback)
 */
core::String stripJsonComments(const core::String &input);

} // namespace lzutf8
} // namespace voxelformat
