/**
 * @file
 */

#pragma once

#include "StbImage.h"
#include "engine-config.h"
#include "io/Stream.h"

namespace image {
namespace format {

namespace PNG {

bool load(io::SeekableReadStream &stream, int length, int &width, int &height, int &components, uint8_t **colors) {
	return StbImage::load(stream, length, width, height, components, colors);
}

bool write(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int components) {
	return StbImage::writePNG(stream, buffer, width, height, components);
}

} // namespace PNG
} // namespace format
} // namespace image
