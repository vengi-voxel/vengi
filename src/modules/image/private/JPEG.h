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
#include <setjmp.h>
#else
#include "StbImage.h"
#endif

namespace image {
namespace format {
namespace JPEG {

#if USE_LIBJPEG
struct LibJPEGErrorHandler {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

static void LibJPGErrorExit(j_common_ptr cinfo) {
	char buffer[JMSG_LENGTH_MAX];
	LibJPEGErrorHandler *jerr = (LibJPEGErrorHandler *)cinfo->err;
	(*cinfo->err->format_message)(cinfo, buffer);
	Log::error("%s", buffer);
	longjmp(jerr->setjmp_buffer, 1);
}

static void LibJPGOutputMessage(j_common_ptr cinfo) {
	char buffer[JMSG_LENGTH_MAX];
	(*cinfo->err->format_message)(cinfo, buffer);
	Log::info("%s", buffer);
}
#endif

bool load(io::ReadStream &stream, int length, int &width, int &height, int &components, uint8_t **colors) {
#if USE_LIBJPEG
	io::BufferedReadWriteStream buffer(stream, length);
	if (buffer.size() != length) {
		Log::debug("Failed to load image: stream size mismatch");
		return false;
	}
	jpeg_decompress_struct cinfo;
	core_memset(&cinfo, 0, sizeof(cinfo));
	LibJPEGErrorHandler jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	cinfo.err->error_exit = LibJPGErrorExit;
	cinfo.err->output_message = LibJPGOutputMessage;

	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		/* Append the filename to the error for easier debugging */
		return false;
	}

	jpeg_create_decompress(&cinfo);

	// this weird cast is needed because libjpeg wants a non-const pointer in older versions
	// and they all only have the LIB_JPEG_VERSION set to 80
	uint8_t *buf = const_cast<uint8_t *>(buffer.getBuffer());
	jpeg_mem_src(&cinfo, buf, length);
	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
		jpeg_destroy_decompress(&cinfo);
		Log::debug("Failed to load image: invalid JPEG header");
		return false;
	}

	// JCS_EXT_RGBA is a libjpeg-turbo extension
#ifdef JCS_ALPHA_EXTENSIONS
	cinfo.out_color_space = JCS_EXT_RGBA;
#else
	cinfo.out_color_space = JCS_RGB;
#endif
	if (!jpeg_start_decompress(&cinfo)) {
		jpeg_destroy_decompress(&cinfo);
		Log::debug("Failed to load image: decompression start failed");
		return false;
	}

	width = cinfo.image_width;
	height = cinfo.image_height;
	const int srcComponents = cinfo.output_components;
	components = 4;

	*colors = (unsigned char *)core_malloc(width * height * components);
	JSAMPROW row_pointer[1];

	if (srcComponents == 4) {
		for (int y = 0; y < height; ++y) {
			row_pointer[0] = *colors + y * width * components;
			jpeg_read_scanlines(&cinfo, row_pointer, 1);
		}
	} else {
		uint8_t *row = (uint8_t *)core_malloc(width * srcComponents);
		for (int y = 0; y < height; ++y) {
			row_pointer[0] = row;
			jpeg_read_scanlines(&cinfo, row_pointer, 1);
			uint8_t *dst = *colors + y * width * components;
			for (int x = 0; x < width; ++x) {
				const uint8_t *src = row + x * srcComponents;
				dst[0] = src[0];
				dst[1] = srcComponents > 1 ? src[1] : src[0];
				dst[2] = srcComponents > 2 ? src[2] : src[0];
				dst[3] = 255;
				dst += components;
			}
		}
		core_free(row);
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
	const bool stripAlpha =
#ifndef JCS_ALPHA_EXTENSIONS
		(components == 4);
#else
		false;
#endif
	if (components == 1) {
		cinfo.input_components = 1;
		cinfo.in_color_space = JCS_GRAYSCALE;
	} else if (components == 3 || stripAlpha) {
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;
	} else {
#ifdef JCS_ALPHA_EXTENSIONS
		cinfo.input_components = components;
		cinfo.in_color_space = JCS_EXT_RGBA;
#else
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;
#endif
	}

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	JSAMPROW row_pointer[1];
	uint8_t *rgbRow = nullptr;
	if (stripAlpha) {
		rgbRow = (uint8_t *)core_malloc(width * 3);
	}
	while (cinfo.next_scanline < cinfo.image_height) {
		if (rgbRow != nullptr) {
			const uint8_t *src = &buffer[cinfo.next_scanline * width * components];
			for (int x = 0; x < width; ++x) {
				rgbRow[x * 3 + 0] = src[x * 4 + 0];
				rgbRow[x * 3 + 1] = src[x * 4 + 1];
				rgbRow[x * 3 + 2] = src[x * 4 + 2];
			}
			row_pointer[0] = rgbRow;
		} else {
			row_pointer[0] = (JSAMPROW)&buffer[cinfo.next_scanline * width * components];
		}
		if (jpeg_write_scanlines(&cinfo, row_pointer, 1) != 1) {
			Log::error("Failed to write scanline %d", cinfo.next_scanline);
			core_free(rgbRow);
			jpeg_destroy_compress(&cinfo);
			free(outbuffer);
			return false;
		}
	}
	core_free(rgbRow);

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
