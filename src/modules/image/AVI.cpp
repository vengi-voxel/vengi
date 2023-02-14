/**
 * @file
 */

#include "AVI.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"

#define AVIF_HASINDEX 0x00000010 // Index at end of file?
#define AVIF_MUSTUSEINDEX 0x00000020
#define AVIF_ISINTERLEAVED 0x00000100
#define AVIF_TRUSTCKTYPE 0x00000800 // Use CKType to find key frames?
#define AVIF_WASCAPTUREFILE 0x00010000
#define AVIF_COPYRIGHTED 0x00020000

namespace image {

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not write avi data: " CORE_STRINGIFY(read));                                                 \
		return false;                                                                                                  \
	}

#define wrapSeek(seek)                                                                                                 \
	if ((seek) == -1) {                                                                                                \
		Log::error("Could not seek in avi stream: " CORE_STRINGIFY(read));                                             \
		return false;                                                                                                  \
	}

class ScopedChunk {
private:
	io::SeekableWriteStream &_stream;
	uint32_t _sizePos = 0;
	bool _success = true;
	int32_t _expectedSize;

public:
	ScopedChunk(io::SeekableWriteStream &stream, uint32_t chunkId, int32_t expectedSize = -1)
		: _stream(stream), _expectedSize(expectedSize) {
		Log::debug("Write chunk type %u", chunkId);
		if (!_stream.writeUInt32(chunkId)) {
			Log::error("Failed to write the chunk type %u", chunkId);
			_success = false;
		}
		_sizePos = _stream.pos();
		if (!_stream.writeUInt32(0)) {
			Log::error("Failed to write the chunk size");
			_success = false;
		}
	}

	~ScopedChunk() {
		const uint32_t dataEnd = _stream.pos();
		const uint32_t delta = dataEnd - _sizePos - sizeof(uint32_t);
		if (_stream.seek(_sizePos) == -1) {
			Log::error("Failed to seek to size pos %u", _sizePos);
			_success = false;
			return;
		}
		if (_expectedSize > 0 && _expectedSize != (int32_t)delta) {
			Log::warn("Unexpected size: %u", delta);
		}
		Log::debug("Write chunk size %u", delta);
		if (!_stream.writeUInt32(delta)) {
			_success = false;
			Log::error("Failed to write chunk size %u", delta);
			return;
		}
		if (_stream.seek(dataEnd) == -1) {
			Log::error("Failed to seek to eos %u", dataEnd);
			_success = false;
			return;
		}
		if (!_success) {
			Log::error("Failed to finish the chunk header");
		}
		// every chunk is 4 byte aligned
		const int pad = (int)(_stream.pos() % 4);
		for (int p = 0; p < pad; ++p) {
			if (!_stream.writeUInt8(0)) {
				_success = false;
				Log::error("Failed to write chunk padding bytes");
				return;
			}
		}
	}

	inline bool success() const {
		return _success;
	}
};

bool AVI::writeHeader(io::SeekableWriteStream &stream, uint32_t riffSize, uint32_t moviSize) {
	wrapBool(stream.writeUInt32(FourCC('R','I','F','F')))
	wrapBool(stream.writeUInt32(riffSize))
	wrapBool(stream.writeUInt32(FourCC('A','V','I',' ')))
	{
		ScopedChunk hdrl(stream, FourCC('L','I','S','T'));
		wrapBool(stream.writeUInt32(FourCC('h','d','r','l')))
		{
			ScopedChunk avih(stream, FourCC('a','v','i','h'), 56);
			wrapBool(stream.writeUInt32(_header.dwMicroSecPerFrame))
			wrapBool(stream.writeUInt32(_header.dwMaxBytesPerSec))
			wrapBool(stream.writeUInt32(_header.dwPaddingGranularity))
			wrapBool(stream.writeUInt32(_header.dwFlags))
			wrapBool(stream.writeUInt32(_header.dwTotalFrames))
			wrapBool(stream.writeUInt32(_header.dwInitialFrames))
			wrapBool(stream.writeUInt32(_header.dwStreams))
			wrapBool(stream.writeUInt32(_header.dwSuggestedBufferSize))
			wrapBool(stream.writeUInt32(_header.dwWidth))
			wrapBool(stream.writeUInt32(_header.dwHeight))
			wrapBool(stream.writeUInt32(_header.dwReserved[0]))
			wrapBool(stream.writeUInt32(_header.dwReserved[1]))
			wrapBool(stream.writeUInt32(_header.dwReserved[2]))
			wrapBool(stream.writeUInt32(_header.dwReserved[3]))
		}

		{
			ScopedChunk strl(stream, FourCC('L','I','S','T'), 148);
			stream.writeUInt32(FourCC('s','t','r','l'));
			{
				AVISTREAMHEADER aviStreamHeader;
				aviStreamHeader.dwRectX2 = _header.dwWidth;
				aviStreamHeader.dwRectY2 = _header.dwHeight;
				aviStreamHeader.dwRate = _state.fps;
				aviStreamHeader.dwLength = _header.dwTotalFrames;

				ScopedChunk strh(stream, FourCC('s','t','r','h'), 56);
				wrapBool(stream.writeUInt32(FourCC('v','i','d','s')))
				wrapBool(stream.writeUInt32(FourCC('M','J','P','G')))
				wrapBool(stream.writeUInt32(aviStreamHeader.dwFlags))
				wrapBool(stream.writeUInt16(aviStreamHeader.wPriority))
				wrapBool(stream.writeUInt16(aviStreamHeader.wLanguage))
				wrapBool(stream.writeUInt32(aviStreamHeader.dwInitialFrames))
				wrapBool(stream.writeUInt32(aviStreamHeader.dwScale))
				wrapBool(stream.writeUInt32(aviStreamHeader.dwRate))
				wrapBool(stream.writeUInt32(aviStreamHeader.dwStart))
				wrapBool(stream.writeUInt32(aviStreamHeader.dwLength))
				wrapBool(stream.writeUInt32(aviStreamHeader.dwSuggestedBufferSize))
				wrapBool(stream.writeUInt32(aviStreamHeader.dwQuality))
				wrapBool(stream.writeUInt32(aviStreamHeader.dwSampleSize))
				wrapBool(stream.writeUInt16(aviStreamHeader.dwRectX1))
				wrapBool(stream.writeUInt16(aviStreamHeader.dwRectY1))
				wrapBool(stream.writeUInt16(aviStreamHeader.dwRectX2))
				wrapBool(stream.writeUInt16(aviStreamHeader.dwRectY2))
			}

			{
				BITMAPINFOHEADER bitmapInfoHeader;
				bitmapInfoHeader.biWidth = (int32_t)_header.dwWidth;
				bitmapInfoHeader.biHeight = (int32_t)_header.dwHeight;
				bitmapInfoHeader.biSizeImage = ((bitmapInfoHeader.biWidth * bitmapInfoHeader.biBitCount / 8 + 3) & 0xFFFFFFFC) * bitmapInfoHeader.biHeight;

				ScopedChunk strf(stream, FourCC('s','t','r','f'));
				wrapBool(stream.writeUInt32(bitmapInfoHeader.biSize))
				wrapBool(stream.writeUInt32(bitmapInfoHeader.biWidth))
				wrapBool(stream.writeUInt32(bitmapInfoHeader.biHeight))
				wrapBool(stream.writeUInt16(bitmapInfoHeader.biPlanes))
				wrapBool(stream.writeUInt16(bitmapInfoHeader.biBitCount))
				wrapBool(stream.writeUInt32(bitmapInfoHeader.biCompression))
				wrapBool(stream.writeUInt32(bitmapInfoHeader.biSizeImage))
				wrapBool(stream.writeUInt32(bitmapInfoHeader.biXPelsPerMeter))
				wrapBool(stream.writeUInt32(bitmapInfoHeader.biYPelsPerMeter))
				wrapBool(stream.writeUInt32(bitmapInfoHeader.biClrUsed))
				wrapBool(stream.writeUInt32(bitmapInfoHeader.biClrImportant))
			}

			{
				ScopedChunk strl(stream, FourCC('L','I','S','T'));
				wrapBool(stream.writeUInt32(FourCC('o','d','m','l')))
				wrapBool(stream.writeUInt32(FourCC('d','m','l','h')))
				wrapBool(stream.writeUInt32(4))
				wrapBool(stream.writeUInt32(_header.dwTotalFrames))
			}
		}
	}

	wrapBool(stream.writeUInt32(FourCC('L','I','S','T')))
	wrapBool(stream.writeUInt32(moviSize))
	wrapBool(stream.writeUInt32(FourCC('m','o','v','i')))
	return true;
}

bool AVI::open(io::SeekableWriteStream &stream, int width, int height, int fps) {
	_state = State();
	_state.fps = fps;
	_state.moviSize = 4; // 4 byte movi id

	_header = AVIMAINHEADER();
	_header.dwMicroSecPerFrame = (uint32_t)(1000000.0f / (float)_state.fps);
	_header.dwFlags = AVIF_HASINDEX | AVIF_ISINTERLEAVED;
	_header.dwHeight = height;
	_header.dwWidth = width;

	wrapBool(writeHeader(stream))

	_indexStream.reset();
	wrapBool(_indexStream.writeUInt32(FourCC('i','d','x','1')))
	wrapBool(_indexStream.writeUInt32(0)) // size - will be filled in close()

	return true;
}

bool AVI::writeJPEGFrame(io::SeekableWriteStream &stream, const uint8_t *jpeg, size_t jpegSize) {
	if (jpeg == nullptr) {
		Log::error("No RGBA image data was given");
		return false;
	}
	if (jpegSize <= 0) {
		Log::error("Invalid dimensions jpeg buffer");
		return false;
	}

	wrapBool(stream.writeUInt32(FourCC('0','0','d','c')))
	const uint32_t paddingSize = (jpegSize % 2 ? 1 : 0);
	const uint32_t chunkSize = jpegSize + paddingSize;
	wrapBool(stream.writeUInt32(chunkSize))
	if (stream.write(jpeg, jpegSize) != (int)jpegSize) {
		Log::error("Failed to write jpeg data");
		return false;
	}
	for (uint32_t p = 0; p < paddingSize; ++p) {
		wrapBool(stream.writeUInt8(0))
	}
	++_header.dwTotalFrames;

	// write index entry
	wrapBool(_indexStream.writeUInt32(FourCC('0','0','d','c')))
	wrapBool(_indexStream.writeUInt32(0x00000010)) // KeyFrame
	wrapBool(_indexStream.writeUInt32(_state.moviSize))
	wrapBool(_indexStream.writeUInt32(chunkSize))
	_state.moviSize += chunkSize + 8; // 00dc + size;

	return true;
}

bool AVI::writeFrame(io::SeekableWriteStream &stream, const uint8_t *rgba, int width, int height) {
	if (rgba == nullptr) {
		Log::error("No RGBA image data was given");
		return false;
	}
	if (width <= 0 || height <= 0) {
		Log::error("Invalid dimensions given");
		return false;
	}

	wrapBool(stream.writeUInt32(FourCC('0','0','d','c')))
	wrapBool(stream.writeUInt32(0))
	const int64_t jpegStart = stream.pos();
	if (!image::Image::writeJPEG(stream, rgba, width, height, 4)) {
		Log::error("Failed to write jpeg data");
		return false;
	}
	const int64_t jpegEnd = stream.pos();
	const uint32_t jpegSize = jpegEnd - jpegStart;
	const uint32_t paddingSize = (jpegSize % 2 ? 1 : 0);
	const uint32_t chunkSize = jpegSize + paddingSize;
	wrapSeek(stream.seek(jpegStart - 4));
	wrapBool(stream.writeUInt32(chunkSize))
	wrapSeek(stream.seek(jpegEnd));
	for (uint32_t p = 0; p < paddingSize; ++p) {
		wrapBool(stream.writeUInt8(0))
	}
	++_header.dwTotalFrames;

	// write index entry
	wrapBool(_indexStream.writeUInt32(FourCC('0','0','d','c')))
	wrapBool(_indexStream.writeUInt32(0x00000010)) // KeyFrame
	wrapBool(_indexStream.writeUInt32(_state.moviSize))
	wrapBool(_indexStream.writeUInt32(chunkSize))
	_state.moviSize += chunkSize + 8; // 00dc + size;

	return true;
}

bool AVI::close(io::SeekableWriteStream &stream) {
	{
		uint32_t indexSize = _header.dwTotalFrames * 16;
		core_assert(_indexStream.size() == indexSize + 8);
		wrapSeek(_indexStream.seek(4, SEEK_SET))
		wrapBool(_indexStream.writeUInt32(indexSize))
	}

	// Append index to end of avi file
	if (stream.write(_indexStream.getBuffer(), _indexStream.size()) != _indexStream.size()) {
		Log::error("Failed to write index buffer");
		return false;
	}

	// Write the real header
	const uint32_t riffSize = stream.size() - 8; // ignore "RIFF" and size
	wrapSeek(stream.seek(0, SEEK_SET))
	wrapBool(writeHeader(stream, riffSize, _state.moviSize))

	wrapSeek(stream.seek(0, SEEK_END))

	Log::info("Wrote %d frames\n", _header.dwTotalFrames);

	return true;
}

#undef wrapSeek
#undef wrapBool

} // namespace image
