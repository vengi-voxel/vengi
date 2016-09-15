#include "VoxelFont.h"
#include "core/App.h"
#include "core/Common.h"
#include "core/UTF8.h"
#define STBTT_ASSERT core_assert
#define STBTT_MALLOC core_malloc
#define STBTT_REALLOC core_realloc
#define STBTT_FREE core_free
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"

namespace voxel {

VoxelFont::~VoxelFont() {
	core_assert_always(_cache.empty());
	shutdown();
}

bool VoxelFont::init(const char* filename, int size, int thickness, bool mergeQuads, const char* glyphs) {
	core_assert_msg(size < 255, "size %i exceeds max vertices position due to limited data type in Vertex class", size);
	core_assert_msg(size > 0, "size must be > 0, but is %i", size);
	const io::FilePtr& file = core::App::getInstance()->filesystem()->open(filename);
	if (!file->exists()) {
		Log::info("Failed to initialize voxel font, %s doesn't exist", filename);
		return false;
	}
	file->read((void**)&_ttfBuffer);
	if (_ttfBuffer == nullptr) {
		Log::info("Failed to initialize voxel font, can not read %s", filename);
		return false;
	}
	const int offset = stbtt_GetFontOffsetForIndex(_ttfBuffer, 0);
	stbtt_InitFont(&_font, _ttfBuffer, offset);
	_size = (int) (size * 1.3f); // FIX: Constant taken out of thin air because fonts get too small.
	_scale = stbtt_ScaleForPixelHeight(&_font, (float)_size);

	_thickness = glm::max(1, thickness);
	int lineGap;
	stbtt_GetFontVMetrics(&_font, &_ascent, &_descent, &lineGap);
	_ascent = (int) (_ascent * _scale + 0.5f);
	_descent = (int) ((-_descent) * _scale + 0.5f);
	_height = (int) ((_ascent - _descent + lineGap) * _scale + 0.5f);

	if (!renderGlyphs(glyphs, mergeQuads)) {
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

	delete[] _ttfBuffer;
	_ttfBuffer = nullptr;

	_size = 0;
	_scale = 0.0f;
	_ascent = 0;
	_descent = 0;
	_height = 0;
	_thickness = 1;
	_spaceWidth = 0;
}

bool VoxelFont::renderGlyphs(const char* string, bool mergeQuads) {
	static const voxel::Voxel& voxel = voxel::createVoxel(voxel::Grass1);
	const char **s = &string;
	int spaceWidth = 0;
	for (int c = core::utf8::next(s); c != -1; c = core::utf8::next(s)) {
		int w;
		int h;
		unsigned char *bitmap = stbtt_GetCodepointBitmap(&_font, 0.0f, _scale, c, &w, &h, 0, 0);
		if (bitmap == nullptr) {
			Log::debug("Could not create voxelfont mesh for character: %i", c);
			continue;
		}

		int ix0, iy0, ix1, iy1;
		stbtt_GetCodepointBitmapBox(&_font, c, 0, _scale, &ix0, &iy0, &ix1, &iy1);

		if (c == ' ') {
			_spaceWidth = w;
		}

		// take the first valid characters with as space width
		if (spaceWidth <= 0) {
			spaceWidth = w;
		}

		voxel::Region region(0, 0, 0, w + 1 + ix0, h + 1 + glm::abs(iy0), _thickness);
		voxel::RawVolume v(region);
		Log::debug("voxelfont: width and height: %i:%i", w, h);
		const int regionH = region.getHeightInCells();
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				if (bitmap[y * w + x] >= 25) {
					for (int i = 0; i < _thickness; ++i) {
						v.setVoxel(glm::ivec3(x + ix0, regionH + iy0 - y, i), voxel);
					}
				}
			}
		}
		stbtt_FreeBitmap(bitmap, nullptr);
		voxel::Mesh* mesh = new voxel::Mesh(8, 8, true);
		voxel::extractCubicMesh(&v, region, mesh, voxel::IsQuadNeeded(false), mergeQuads, mergeQuads);
		if (mesh->getNoOfIndices() > 0) {
			_cache[c] = mesh;
		} else {
			Log::debug("Could not extract mesh for character %i", c);
		}
	}
	if (_spaceWidth == 0) {
		_spaceWidth = spaceWidth;
	}
	return true;
}

int VoxelFont::render(const char* string, std::vector<glm::vec4>& pos, std::vector<uint32_t>& indices) {
	const char **s = &string;
	int xBase = 0;
	int yBase = 0;
	int charCount = 0;
	for (int c = core::utf8::next(s); c != -1; c = core::utf8::next(s), ++charCount) {
		if (c == ' ') {
			xBase += _spaceWidth;
			continue;
		} else if (c == '\n') {
			xBase = 0;
			yBase += _height;
			continue;
		}

		auto i = _cache.find(c);
		if (i == _cache.end()) {
			xBase += _size;
			Log::trace("Could not find character glyph cache for %i", c);
			continue;
		}

		int x = xBase;
		int y = yBase;
		int advanceWidth;
		int leftSideBearing;
		stbtt_GetCodepointHMetrics(&_font, c, &advanceWidth, &leftSideBearing);
		const int advance = (int) (advanceWidth * _scale + 0.5f);
		xBase += advance;

		const voxel::Mesh* mesh = i->second;
		const voxel::IndexType* meshIndices = mesh->getRawIndexData();
		const voxel::Vertex* meshVertices = mesh->getRawVertexData();
		const size_t meshNumberIndices = mesh->getNoOfIndices();
		core_assert(meshNumberIndices > 0);
		const size_t meshNumberVertices = mesh->getNoOfVertices();
		core_assert(meshNumberVertices > 0);

		const size_t positionSize = pos.size();
		const size_t indicesSize = indices.size();
		pos.reserve(positionSize + meshNumberVertices);
		indices.reserve(indicesSize + meshNumberIndices);

		for (size_t i = 0; i < meshNumberVertices; ++i) {
			glm::vec4 vp = glm::vec4(meshVertices[i].position, 1.0f);
			vp.x += x;
			vp.y += y;
			pos.push_back(vp);
		}
		for (size_t i = 0; i < meshNumberIndices; ++i) {
			// offset by the already added vertices
			indices.push_back(meshIndices[i] + positionSize);
		}
	}
	return charCount;
}

}
