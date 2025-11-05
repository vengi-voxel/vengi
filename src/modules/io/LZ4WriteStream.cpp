/**
 * @file
 */

#include "LZ4WriteStream.h"

#ifdef USE_LZ4

#include "core/Log.h"
#include "core/StandardLib.h"
#include <lz4.h>
#include <lz4frame.h>

namespace io {

LZ4WriteStream::LZ4WriteStream(io::WriteStream &outStream, int level) : _outStream(outStream) {
	LZ4F_cctx *cctx = nullptr;
	LZ4F_errorCode_t result = LZ4F_createCompressionContext(&cctx, LZ4F_VERSION);
	if (LZ4F_isError(result)) {
		Log::error("Failed to create LZ4 compression context: %s", LZ4F_getErrorName(result));
		_stream = nullptr;
		return;
	}

	_stream = cctx;

	// Set up preferences based on compression level
	LZ4F_preferences_t prefs;
	core_memset(&prefs, 0, sizeof(prefs));
	prefs.frameInfo.contentSize = 0; // Unknown content size
	prefs.frameInfo.blockMode = LZ4F_blockIndependent;

	// Map level: 0 = fast, 1-9 = HC compression (max is 12 for LZ4HC)
	if (level <= 0) {
		prefs.compressionLevel = 0; // Fast compression
	} else {
		prefs.compressionLevel = core_min(level, 12); // LZ4HC max level
	}

	// Write frame header
	const size_t headerSize = LZ4F_compressBegin(cctx, _out, sizeof(_out), &prefs);
	if (LZ4F_isError(headerSize)) {
		Log::error("Failed to begin LZ4 compression: %s", LZ4F_getErrorName(headerSize));
		LZ4F_freeCompressionContext(cctx);
		_stream = nullptr;
		return;
	}

	if (_outStream.write(_out, headerSize) != (int)headerSize) {
		Log::error("Failed to write LZ4 frame header");
		LZ4F_freeCompressionContext(cctx);
		_stream = nullptr;
		return;
	}

	_pos += headerSize;
}

LZ4WriteStream::~LZ4WriteStream() {
	LZ4WriteStream::flush();
	if (_stream != nullptr) {
		LZ4F_freeCompressionContext((LZ4F_cctx *)_stream);
		_stream = nullptr;
	}
}

int LZ4WriteStream::write(const void *buf, size_t size) {
	if (size == 0) {
		return 0;
	}

	if (_stream == nullptr) {
		return -1;
	}

	LZ4F_cctx *cctx = (LZ4F_cctx *)_stream;

	// Process the data in chunks if necessary
	const uint8_t *inputBuf = (const uint8_t *)buf;
	size_t totalWritten = 0;

	while (totalWritten < size) {
		// Calculate chunk size - find the maximum input size that fits in our output buffer
		// LZ4F_compressBound tells us how much output space we need for a given input size
		size_t chunkSize = size - totalWritten;
		size_t maxCompressedSize = LZ4F_compressBound(chunkSize, nullptr);

		// If the compressed bound is too large, reduce chunk size
		while (maxCompressedSize > sizeof(_out) && chunkSize > 1) {
			chunkSize /= 2;
			maxCompressedSize = LZ4F_compressBound(chunkSize, nullptr);
		}

		if (maxCompressedSize > sizeof(_out)) {
			Log::error("LZ4 compression error: output buffer too small even for minimal chunk");
			return -1;
		}

		// Compress this chunk
		const size_t compressedSize = LZ4F_compressUpdate(cctx, _out, sizeof(_out), inputBuf + totalWritten, chunkSize,
														  nullptr // Use default options
		);

		if (LZ4F_isError(compressedSize)) {
			Log::error("LZ4 compression error: %s", LZ4F_getErrorName(compressedSize));
			return -1;
		}

		if (compressedSize > 0) {
			if (_outStream.write(_out, compressedSize) != (int)compressedSize) {
				return -1;
			}
			_pos += compressedSize;
		}

		totalWritten += chunkSize;
	}

	return (int)size;
}

bool LZ4WriteStream::flush() {
	if (_stream == nullptr || _finalized) {
		return _outStream.flush();
	}

	LZ4F_cctx *cctx = (LZ4F_cctx *)_stream;

	// Finalize the frame
	const size_t finalSize = LZ4F_compressEnd(cctx, _out, sizeof(_out), nullptr);
	if (LZ4F_isError(finalSize)) {
		Log::error("LZ4 compression end error: %s", LZ4F_getErrorName(finalSize));
		return false;
	}

	if (finalSize > 0) {
		if (_outStream.write(_out, finalSize) != (int)finalSize) {
			return false;
		}
		_pos += finalSize;
	}

	_finalized = true;
	return _outStream.flush();
}

} // namespace io

#endif // USE_LZ4
