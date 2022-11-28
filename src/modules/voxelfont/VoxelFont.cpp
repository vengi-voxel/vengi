/**
 * @file
 */

#include "VoxelFont.h"
#include "app/App.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "io/Filesystem.h"
#include "voxel/RawVolumeWrapper.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function" // for stb_truetype.h
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function" // for stb_truetype.h
#endif

#define STBTT_assert core_assert
#define STBTT_malloc(x, u) ((void)(u), core_malloc(x))
#define STBTT_free(x, u) ((void)(u), core_free(x))
#define STBTT_realloc(x, u) ((void)(u), core_realloc(x))
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "external/stb_truetype.h"

namespace voxelfont {

VoxelFont::~VoxelFont() {
	shutdown();
}

bool VoxelFont::init(const char *filename) {
	if (_filename == filename) {
		return true;
	}
	shutdown();
	const io::FilePtr &file = io::filesystem()->open(filename);
	if (!file->exists()) {
		Log::info("Failed to initialize voxel font, %s doesn't exist", filename);
		return false;
	}
	delete[] _ttfBuffer;
	file->read((void **)&_ttfBuffer);
	if (_ttfBuffer == nullptr) {
		Log::info("Failed to initialize voxel font, can not read %s", filename);
		return false;
	}
	const int offset = stbtt_GetFontOffsetForIndex(_ttfBuffer, 0);
	_font = new stbtt_fontinfo();
	stbtt_InitFont(_font, _ttfBuffer, offset);

	Log::info("Initialized voxel font for %s", filename);
	_filename = filename;
	return true;
}

void VoxelFont::shutdown() {
	delete _font;
	_font = nullptr;

	delete[] _ttfBuffer;
	_ttfBuffer = nullptr;

	_filename = "";
}

int VoxelFont::renderCharacter(int codepoint, uint8_t size, int thickness, const glm::ivec3 &pos,
								voxel::RawVolumeWrapper &volume, const voxel::Voxel &voxel) {
	const float scale = stbtt_ScaleForPixelHeight(_font, (float)size);
	int w;
	int h;
	unsigned char *bitmap = stbtt_GetCodepointBitmap(_font, 0.0f, scale, codepoint, &w, &h, nullptr, nullptr);
	thickness = core_max(1, thickness);

	if (bitmap == nullptr) {
		Log::warn("Could not create voxelfont mesh for character: %i", codepoint);
		return 0;
	}

	int ix0, iy0, ix1, iy1;
	stbtt_GetCodepointBitmapBox(_font, codepoint, 0.0f, scale, &ix0, &iy0, &ix1, &iy1);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			// antialiasing
			if (bitmap[y * w + x] >= 25) {
				glm::ivec3 v(x + ix0, h - y, 0);
				for (int i = 0; i < thickness; ++i) {
					v.z = i;
					volume.setVoxel(pos + v, voxel);
				}
			}
		}
	}
	stbtt_FreeBitmap(bitmap, nullptr);
	return w;
}

} // namespace voxelfont

#ifdef __clang__
#pragma clang diagnostic pop
#endif
