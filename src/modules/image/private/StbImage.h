/**
 * @file
 */

#pragma once

#include "io/Stream.h"

#define STBI_NO_STDIO
#include <stb_image.h>

namespace image {
namespace format {
namespace StbImage {

static int stream_read(void *user, char *data, int size) {
	io::SeekableReadStream *stream = (io::SeekableReadStream *)user;
	const int readSize = stream->read(data, size);
	// prevent endless loops on errors
	if (readSize < 0) {
		return 0;
	}
	return readSize;
}

static void stream_skip(void *user, int n) {
	io::SeekableReadStream *stream = (io::SeekableReadStream *)user;
	stream->skip(n);
}

static int stream_eos(void *user) {
	io::SeekableReadStream *stream = (io::SeekableReadStream *)user;
	return stream->eos() ? 1 : 0;
}

bool load(io::SeekableReadStream &stream, int length, int &width, int &height, int &components,
				 uint8_t **colors) {
	stbi_io_callbacks clbk;
	clbk.read = stream_read;
	clbk.skip = stream_skip;
	clbk.eof = stream_eos;
	*colors = stbi_load_from_callbacks(&clbk, &stream, &width, &height, &components, STBI_rgb_alpha);
	// we are always using rgba
	components = 4;
	return true;
}

} // namespace StbImage
} // namespace format
} // namespace image
