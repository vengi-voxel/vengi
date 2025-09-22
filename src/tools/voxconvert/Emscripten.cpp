/**
 * @file
 */

#include "core/String.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FormatDescription.h"
#include "palette/PaletteFormatDescription.h"
#include "voxelformat/VolumeFormat.h"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE
#endif

extern "C" EMSCRIPTEN_KEEPALIVE const char *get_supported_formats_json() {
	static core::String formats;
	io::BufferedReadWriteStream stream;
	stream.writeString("{\"voxels\":[", false);
	io::format::writeJson(stream, voxelformat::voxelLoad(),
						  {{"thumbnail_embedded", VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED},
						   {"palette_embedded", VOX_FORMAT_FLAG_PALETTE_EMBEDDED},
						   {"mesh", VOX_FORMAT_FLAG_MESH},
						   {"animation", VOX_FORMAT_FLAG_ANIMATION},
						   {"save", FORMAT_FLAG_SAVE}});
	stream.writeString("],\"images\":[", false);
	io::format::writeJson(stream, io::format::images(), {{"save", FORMAT_FLAG_SAVE}});
	stream.writeString("],\"palettes\":[", false);
	io::format::writeJson(stream, palette::palettes(), {{"save", FORMAT_FLAG_SAVE}});
	stream.writeString("]}", false);
	formats = core::String((const char *)stream.getBuffer(), stream.size());
	return formats.c_str();
}
