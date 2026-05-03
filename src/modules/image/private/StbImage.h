/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "io/Stream.h"

#define STBI_NO_STDIO
#include <stb_image.h>

#define STBI_WRITE_NO_STDIO
#include <stb_image_write.h>

namespace image {
namespace format {
namespace StbImage {

class StbReadStream {
private:
	io::ReadStream &_stream;
public:
	StbReadStream(io::ReadStream &stream) : _stream(stream) {
	}

	bool eos() const {
		return _stream.eos();
	}
	int read(void *data, int size) {
		return (int)_stream.read(data, size);
	}
	void skip(int n) {
		for (int i = 0; i < n; ++i) {
			uint8_t ignore;
			_stream.readUInt8(ignore);
		}
	}
};

static int stream_read(void *user, char *data, int size) {
	StbReadStream *stream = (StbReadStream *)user;
	const int readSize = stream->read(data, size);
	// prevent endless loops on errors
	if (readSize < 0) {
		return 0;
	}
	return readSize;
}

static void stream_skip(void *user, int n) {
	StbReadStream *stream = (StbReadStream *)user;
	stream->skip(n);
}

static int stream_eos(void *user) {
	StbReadStream *stream = (StbReadStream *)user;
	return stream->eos() ? 1 : 0;
}

bool load(io::ReadStream &stream, int length, int &width, int &height, int &components, uint8_t **colors) {
	stbi_io_callbacks clbk;
	StbReadStream streamWrapper(stream);
	clbk.read = stream_read;
	clbk.skip = stream_skip;
	clbk.eof = stream_eos;
	*colors = stbi_load_from_callbacks(&clbk, &streamWrapper, &width, &height, &components, STBI_rgb_alpha);
	// we are always using rgba
	components = 4;
	return true;
}

static void stream_write_func(void *context, void *data, int size) {
	io::SeekableWriteStream *stream = (io::SeekableWriteStream *)context;
	int64_t written = stream->write(data, size);
	if (written != size) {
		Log::error("Failed to write to image stream: %i vs %i", (int)written, size);
	}
}

bool writePNG(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int components) {
	return stbi_write_png_to_func(stream_write_func, &stream, width, height, components, buffer, width * components) !=
		   0;
}

bool writeJPEG(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int components,
			   int quality = 100) {
	return stbi_write_jpg_to_func(stream_write_func, &stream, width, height, components, buffer, quality) != 0;
}

} // namespace StbImage
} // namespace format
} // namespace image
