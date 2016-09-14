#include "VoxelFont.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace voxel {

VoxelFont::~VoxelFont() {
	for (auto e : _cache) {
		delete[] e.second.buffer;
	}
	_cache.clear();
}

// TODO: use RawVolume and rasterize bitmap for a char. Loop over the pixels and fill the RawVolume with voxels.
bool VoxelFont::renderString(const char* string, int size, const glm::vec4& color, uint8_t *buffer, size_t bufferSize) {
#if 0
	// TODO: there is also 3d support in stb_truetype
	const int offset = stbtt_GetFontOffsetForIndex(ttf_buffer, 0);
	stbtt_InitFont(&_font, ttf_buffer, offset);
	const float scale = stbtt_ScaleForPixelHeight(&_font, size);
	for (int c = utf8_begin(&string); c != 0; c = utf8_next(&string)) {
		unsigned char *bitmap = stbtt_GetCodepointBitmap(&_font, 0.0f, scale, c, &w, &h, 0, 0);
		stbtt_FreeBitmap(bitmap);
	}
#endif
	return false;
}

}
