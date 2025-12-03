/**
 * @file
 */

#include "VoxelFont.h"
#include "app/App.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Unicode.h"
#include "io/Filesystem.h"
#include "voxel/RawVolumeWrapper.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function" // for stb_truetype.h
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function" // for stb_truetype.h
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced local function has been removed
#pragma warning(disable : 4127) // conditional expression is constant
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

bool VoxelFont::init(const core::String &filename) {
	if (_filename == filename) {
		return true;
	}
	shutdown();
	const io::FilePtr &file = io::filesystem()->open(filename);
	if (!file->exists()) {
		Log::info("Failed to initialize voxel font, %s doesn't exist", filename.c_str());
		return false;
	}
	delete[] _ttfBuffer;
	file->read((void **)&_ttfBuffer);
	if (_ttfBuffer == nullptr) {
		Log::info("Failed to initialize voxel font, can not read %s", filename.c_str());
		return false;
	}
	const int offset = stbtt_GetFontOffsetForIndex(_ttfBuffer, 0);
	_font = new stbtt_fontinfo();
	stbtt_InitFont(_font, _ttfBuffer, offset);

	Log::info("Initialized voxel font for %s", filename.c_str());
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

void VoxelFont::dimensions(const char *string, uint8_t size, int &w, int &h) const {
	const char **s = &string;
	const float scale = stbtt_ScaleForPixelHeight(_font, (float)size);
	int ix0, iy0, ix1, iy1;
	w = 0;
	h = 0;
	for (int c = core::unicode::next(s); c != -1; c = core::unicode::next(s)) {
		stbtt_GetCodepointBitmapBox(_font, c, scale, scale, &ix0, &iy0, &ix1, &iy1);
		w += (ix1 - ix0);
		h = core_max(h, iy1 - iy0);
	}
}

int VoxelFont::renderCharacter(int codepoint, uint8_t size, int thickness, const glm::ivec3 &pos,
							   voxel::RawVolumeWrapper &volume, const voxel::Voxel &voxel, math::Axis axis) {
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

	const int widthIndex = math::getIndexForAxis(axis);
	glm::ivec3 dim;
	dim[(widthIndex + 0) % 3] = w;
	dim[(widthIndex + 1) % 3] = h;
	dim[(widthIndex + 2) % 3] = thickness;

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			// antialiasing
			if (bitmap[y * w + x] >= 25) {
				glm::ivec3 v;
				v[(widthIndex + 0) % 3] = x + ix0;
				v[(widthIndex + 1) % 3] = dim[(widthIndex + 1) % 3] - 1 - y;
				v[(widthIndex + 2) % 3] = 0;
				for (int z = 0; z < thickness; ++z) {
					v[(widthIndex + 2) % 3] = z;
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

#ifdef _MSC_VER
#pragma warning(pop)
#endif
