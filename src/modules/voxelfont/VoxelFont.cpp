/**
 * @file
 */

#include "VoxelFont.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "core/Common.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function" // for stb_truetype.h
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function" // for stb_truetype.h
#endif

#define STBTT_assert core_assert
#define STBTT_malloc(x,u)   ((void)(u), core_malloc(x))
#define STBTT_free(x,u)     ((void)(u), core_free(x))
#define STBTT_realloc(x,u)  ((void)(u), core_realloc(x))
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/MaterialColor.h"

namespace voxel {

VoxelFont::~VoxelFont() {
	core_assert_always(_cache.empty());
	shutdown();
}

void VoxelFont::getGlyphMetrics(int c, int& advance, int& xOffset, int& yOffset, int& ascent) {
	int advanceWidth, leftSideBearing;
	stbtt_GetCodepointHMetrics(_font, c, &advanceWidth, &leftSideBearing);
	advance = (int) (advanceWidth * _scale + 0.5f);
	int ix0, iy0, ix1, iy1;
	stbtt_GetCodepointBitmapBox(_font, c, 0, _scale, &ix0, &iy0, &ix1, &iy1);
	xOffset = ix0;
	yOffset = iy0;

	int descent, lineGap;
	stbtt_GetFontVMetrics(_font, &ascent, &descent, &lineGap);
	ascent = (int) (ascent * _scale + 0.5f);
}

int VoxelFont::stringWidth(const char *str, int len) const {
	int width = 0;
	int i = 0;
	const char **s = &str;
	while (str[i] && i < len) {
		int c = core::utf8::next(s);
		if (c == -1) {
			break;
		}
		int advanceWidth, leftSideBearing;
		stbtt_GetCodepointHMetrics(_font, c, &advanceWidth, &leftSideBearing);
		width += (int) (advanceWidth * _scale + 0.5f);
	}
	return width;
}

bool VoxelFont::init(const char* filename, int size, int thickness, uint8_t optionMask, const char* glyphs) {
	_optionMask = optionMask;
	core_assert_msg(size < 255, "size %i exceeds max vertices position due to limited data type in Vertex class", size);
	core_assert_msg(size > 0, "size must be > 0, but is %i", size);
	const io::FilePtr& file = core::App::getInstance()->filesystem()->open(filename);
	if (!file->exists()) {
		Log::info("Failed to initialize voxel font, %s doesn't exist", filename);
		return false;
	}
	delete[] _ttfBuffer;
	file->read((void**)&_ttfBuffer);
	if (_ttfBuffer == nullptr) {
		Log::info("Failed to initialize voxel font, can not read %s", filename);
		return false;
	}
	const int offset = stbtt_GetFontOffsetForIndex(_ttfBuffer, 0);
	_font = new stbtt_fontinfo();
	stbtt_InitFont(_font, _ttfBuffer, offset);
	_size = size;
	_scale = stbtt_ScaleForPixelHeight(_font, (float)_size);

	_thickness = glm::max(1, thickness);
	stbtt_GetFontVMetrics(_font, &_ascent, &_descent, &_lineGap);
	_ascent = (int) (_ascent * _scale + 0.5f);
	_descent = (int) ((-_descent) * _scale + 0.5f);
	_height = (int) ((_ascent - _descent + _lineGap) * _scale + 0.5f);

	Log::debug("ascent: %i, descent: %i, linegap: %i", _ascent, _descent, _lineGap);

	if (!renderGlyphs(glyphs)) {
		Log::info("Failed to initialize voxel font, failed to render glyphs for %s", filename);
		return false;
	}
	Log::info("Initialized voxel font for %s", filename);
	return true;
}

void VoxelFont::shutdown() {
	for (auto e : _cache) {
		delete e.second;
	}
	_cache.clear();

	delete _font;
	_font = nullptr;

	delete[] _ttfBuffer;
	_ttfBuffer = nullptr;

	_size = 0;
	_scale = 0.0f;
	_ascent = 0;
	_descent = 0;
	_height = 0;
	_lineGap = 0;
	_thickness = 1;
	_spaceWidth = 0;
}

bool VoxelFont::renderGlyphs(const char* string) {
	static const voxel::Voxel& voxel = voxel::createColorVoxel(VoxelType::Generic, 0);
	const char **s = &string;
	int spaceWidth = 0;
	int chars = 0;
	const bool mergeQuads = _optionMask & MergeQuads;
	for (int c = core::utf8::next(s); c != -1; c = core::utf8::next(s)) {
		int w;
		int h;
		unsigned char *bitmap = stbtt_GetCodepointBitmap(_font, 0.0f, _scale, c, &w, &h, nullptr, nullptr);
		if (bitmap == nullptr) {
			Log::warn("Could not create voxelfont mesh for character: %i", c);
			continue;
		}

		int ix0, iy0, ix1, iy1;
		stbtt_GetCodepointBitmapBox(_font, c, 0.0f, _scale, &ix0, &iy0, &ix1, &iy1);
		Log::debug("w: %i, h: %i, scale: %f, ix0: %i, iy0: %i, ix1: %i, iy1: %i",
				w, h, _scale, ix0, iy0, ix1, iy1);

		if (c == ' ') {
			_spaceWidth = w;
		}

		// take the first valid characters with as space width
		spaceWidth += w;

		voxel::Region region(0, 0, 0, w + ix0, h + glm::abs(iy0), _thickness - 1);
		voxel::RawVolume v(region);
		Log::debug("voxelfont: width and height: %i:%i", w, h);
		const int regionH = region.getHeightInCells();
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				if (bitmap[y * w + x] >= 25) {
					for (int i = 0; i < _thickness; ++i) {
						v.setVoxel(glm::ivec3(x + ix0, regionH - y, i), voxel);
					}
				}
			}
		}
		stbtt_FreeBitmap(bitmap, nullptr);
		voxel::Mesh* mesh = new voxel::Mesh(8, 8, true);
		voxel::extractCubicMesh(&v, region, mesh, voxel::IsQuadNeeded(), mergeQuads, mergeQuads);
		if (mesh->getNoOfIndices() > 0) {
			_cache[c] = mesh;
			++chars;
		} else {
			Log::debug("Could not extract mesh for character %i", c);
		}
	}
	if (_spaceWidth == 0 && chars > 0) {
		_spaceWidth = spaceWidth / chars;
	}
	return true;
}

int VoxelFont::render(const char* string, std::vector<glm::vec4>& pos, std::vector<uint32_t>& indices) {
	return render(string, pos, indices, [] (const voxel::VoxelVertex& vertex, std::vector<glm::vec4>& pos, int x, int y) {
		glm::vec4 vp(vertex.position, 1.0f);
		vp.x += x;
		vp.y += y;
		pos.push_back(vp);
	});
}

int VoxelFont::render(const char* string, std::vector<voxel::VoxelVertex>& vertices, std::vector<uint32_t>& indices) {
	return render(string, vertices, indices, [] (const voxel::VoxelVertex& vertex, std::vector<voxel::VoxelVertex>& vertices, int x, int y) {
		voxel::VoxelVertex copy = vertex;
		copy.position.x += x;
		copy.position.y += y;
		vertices.push_back(copy);
	});
}

}

#ifdef __clang__
#pragma clang diagnostic pop
#endif
