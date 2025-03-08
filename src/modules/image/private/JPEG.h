/**
 * @file
 */

#pragma once

#include "StbImage.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/collection/DynamicArray.h"
#include "engine-config.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"

#ifdef USE_LIBJPEG
#include <jpeglib.h>
#endif

namespace image {
namespace format {
namespace JPEG {

bool load(io::SeekableReadStream &stream, int length, int &width, int &height, int &components, uint8_t **colors) {
#ifdef USE_LIBJPEG
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	io::BufferedReadWriteStream buffer(stream, length);
	jpeg_mem_src(&cinfo, buffer.getBuffer(), length);
	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
		jpeg_destroy_decompress(&cinfo);
		Log::debug("Failed to load image: invalid JPEG header");
		return false;
	}

	if (!jpeg_start_decompress(&cinfo)) {
		jpeg_destroy_decompress(&cinfo);
		Log::debug("Failed to load image: decompression start failed");
		return false;
	}

	width = cinfo.output_width;
	height = cinfo.output_height;
	components = 4; // Always RGBA

	*colors = (unsigned char *)core_malloc(width * height * components);
	core::DynamicArray<unsigned char> row_buffer(width * cinfo.output_components);

	for (int y = 0; y < height; ++y) {
		JSAMPROW row_pointer = &row_buffer[0];
		if (jpeg_read_scanlines(&cinfo, &row_pointer, 1) != 1) {
			jpeg_destroy_decompress(&cinfo);
			core_free(*colors);
			*colors = nullptr;
			Log::debug("Failed to load image: scanline read error");
			return false;
		}
		unsigned char *dst = *colors + y * width * components;
		for (int x = 0; x < width; ++x) {
			dst[x * components + 0] = row_buffer[x * cinfo.output_components + 0];
			dst[x * components + 1] = row_buffer[x * cinfo.output_components + (cinfo.output_components > 1 ? 1 : 0)];
			dst[x * components + 2] = row_buffer[x * cinfo.output_components + (cinfo.output_components > 2 ? 2 : 0)];
			dst[x * components + 3] = 255; // Default alpha to 255
		}
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	return true;
#else
	return StbImage::load(stream, length, width, height, components, colors);
#endif
}

} // namespace JPEG
} // namespace format
} // namespace image
