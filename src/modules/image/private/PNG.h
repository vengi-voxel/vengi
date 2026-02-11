/**
 * @file
 */

#pragma once

#include "StbImage.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/collection/Buffer.h"
#include "engine-config.h"
#include "io/Stream.h"

#ifdef USE_LIBPNG
#define PNG_SKIP_SETJMP_CHECK // libpng 1.2.50
#include <png.h>
#endif

namespace image {
namespace format {

namespace PNG {

#ifdef USE_LIBPNG
static void pngErrorHandler(png_structp png, png_const_charp msg) {
	// Handle error by simply logging it and setting an error flag
	bool *hasError = (bool *)png_get_error_ptr(png);
	*hasError = true;
	Log::error("libpng error: %s", msg);
}

static void pngReadFunc(png_structp png, png_bytep data, png_size_t pnglength) {
	io::SeekableReadStream *s = (io::SeekableReadStream *)png_get_io_ptr(png);
	s->read(data, pnglength);
}

static void pngWriteFunc(png_structp png, png_bytep data, png_size_t pnglength) {
	io::SeekableWriteStream *s = (io::SeekableWriteStream *)png_get_io_ptr(png);
	s->write(data, pnglength);
}
#endif

bool load(io::SeekableReadStream &stream, int length, int &width, int &height, int &components, uint8_t **colors) {
#if 0 // libpng reading is slower than stb_image
// #ifdef USE_LIBPNG
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png) {
		return false;
	}

	png_infop info = png_create_info_struct(png);
	if (!info) {
		png_destroy_read_struct(&png, nullptr, nullptr);
		return false;
	}

	bool hasError = false;
	png_set_error_fn(png, &hasError, pngErrorHandler, nullptr);
	png_set_read_fn(png, &stream, pngReadFunc);

	png_read_info(png, info);
	width = png_get_image_width(png, info);
	height = png_get_image_height(png, info);
	int pngcomponents = png_get_channels(png, info);
	int color_type = png_get_color_type(png, info);
	int bit_depth = png_get_bit_depth(png, info);
	Log::debug("Got PNG image: %i x %i, %i components, color type: %i, bit depth: %i", width, height, pngcomponents,
			   color_type, bit_depth);

	// Expand grayscale images to RGB
	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(png);
	}

	// Expand palette images to RGB
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png);
	}

	// Ensure all images have an alpha channel
	if (!(color_type & PNG_COLOR_MASK_ALPHA)) {
		png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
	}

	// Convert bit depths lower than 8 to 8-bit
	if (bit_depth < 8) {
		png_set_packing(png);
	} else if (bit_depth == 16) {
		png_set_strip_16(png);
	}
	png_read_update_info(png, info);

	components = 4;
	*colors = (uint8_t *)core_malloc(width * height * components);
	core::DynamicArray<png_bytep> row_pointers;
	row_pointers.resize(height);
	for (int y = 0; y < height; ++y) {
		row_pointers[y] = *colors + y * width * components;
	}
	png_read_image(png, row_pointers.data());
	png_destroy_read_struct(&png, &info, nullptr);
	return !hasError;
#else
	return StbImage::load(stream, length, width, height, components, colors);
#endif
}

bool write(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int components) {
#ifdef USE_LIBPNG
	// libpng writing is faster than stb_image
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png) {
		return false;
	}

	png_infop info = png_create_info_struct(png);
	if (!info) {
		png_destroy_write_struct(&png, nullptr);
		return false;
	}

	bool hasError = false;
	stream.reserve(width * height * components);
	png_set_error_fn(png, &hasError, pngErrorHandler, nullptr);
	png_set_write_fn(png, &stream, pngWriteFunc, nullptr);

	png_set_IHDR(png, info, width, height, 8, (components == 4) ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png, info);

	core::Buffer<png_bytep> row_pointers;
	row_pointers.resize(height);
	for (int y = 0; y < height; ++y) {
		row_pointers[y] = const_cast<png_bytep>(buffer) + y * width * components;
	}
	png_write_image(png, row_pointers.data());
	png_write_end(png, nullptr);

	png_destroy_write_struct(&png, &info);
	return !hasError;
#else
	return StbImage::writePNG(stream, buffer, width, height, components);
#endif
}

} // namespace PNG
} // namespace format
} // namespace image
