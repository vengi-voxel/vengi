/**
 * @file
 */

#pragma once

#include "io/MemoryReadStream.h"

namespace io {

/**
 * @brief Read from a zip input stream and save in a local buffer
 *
 * @see ZipReadStream
 * @see ZipWriteStream
 * @see MemoryReadStream
 */
class BufferedZipReadStream : public MemoryReadStream {
public:
	/**
	 * @param stream The input stream with the compressed data
	 * @param zipSize The size of the compressed data in bytes
	 * @param maxUncompressedSize The estimated max uncompressed size
	 */
	BufferedZipReadStream(SeekableReadStream &stream, size_t zipSize, size_t maxUncompressedSize);

	~BufferedZipReadStream();
};

} // namespace io
