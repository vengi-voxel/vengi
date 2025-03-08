/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "core/StandardLib.h"
#include "engine-config.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"

#ifdef USE_LIBJPEG
#include <jpeglib.h>
#include <stdlib.h> // free
#else
#include "StbImage.h"
#endif

namespace image {
namespace format {
namespace JPEG {

bool load(io::SeekableReadStream &stream, int length, int &width, int &height, int &components, uint8_t **colors) {
#if USE_LIBJPEG
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;

	jpeg_create_decompress(&cinfo);
	cinfo.err = jpeg_std_error(&jerr);

	io::BufferedReadWriteStream buffer(stream, length);
	jpeg_mem_src(&cinfo, buffer.getBuffer(), length);
	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
		jpeg_destroy_decompress(&cinfo);
		Log::debug("Failed to load image: invalid JPEG header");
		return false;
	}

	cinfo.out_color_space = JCS_EXT_RGBA;
	if (!jpeg_start_decompress(&cinfo)) {
		jpeg_destroy_decompress(&cinfo);
		Log::debug("Failed to load image: decompression start failed");
		return false;
	}

	width = cinfo.image_width;
	height = cinfo.image_height;
	components = cinfo.output_components;

	*colors = (unsigned char *)core_malloc(width * height * components);
	JSAMPROW row_pointer[1];

	for (int y = 0; y < height; ++y) {
		row_pointer[0] = *colors + y * width * components;
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	return true;
#else
	return StbImage::load(stream, length, width, height, components, colors);
#endif
}

bool write(io::SeekableWriteStream &stream, const uint8_t *buffer, int width, int height, int components, int quality) {
#ifdef USE_LIBJPEG
	// libjpeg writing is fast than stb_image
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned char *outbuffer = nullptr;
	unsigned long outsize = 0;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	jpeg_mem_dest(&cinfo, &outbuffer, &outsize);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = components;
	if (components == 1) {
		cinfo.in_color_space = JCS_GRAYSCALE;
	} else if (components == 3) {
		cinfo.in_color_space = JCS_RGB;
	} else {
		cinfo.in_color_space = JCS_EXT_RGBA;
	}

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW row_pointer[1];
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = (JSAMPROW)&buffer[cinfo.next_scanline * width * components];
		if (jpeg_write_scanlines(&cinfo, row_pointer, 1) != 1) {
			Log::error("Failed to write scanline %d", cinfo.next_scanline);
			jpeg_destroy_compress(&cinfo);
			free(outbuffer);
			return false;
		}
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	if (stream.write(outbuffer, outsize) == -1) {
		free(outbuffer);
		Log::error("Failed to write JPEG image to stream");
		return false;
	}
	free(outbuffer);

	return true;
#else
	// stbi fallback implementation
	return StbImage::writeJPEG(stream, buffer, width, height, components);
#endif
}

} // namespace JPEG
} // namespace format
} // namespace image
