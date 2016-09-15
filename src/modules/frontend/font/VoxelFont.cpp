#include "VoxelFont.h"
#include "core/App.h"
#include "core/Log.h"
#include "core/UTF8.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"

namespace frontend {

VoxelFont::~VoxelFont() {
	shutdown();
}

bool VoxelFont::init(const char* filename, int size, const char* glyphs) {
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

	delete[] _ttfBuffer;
	_ttfBuffer = nullptr;

	_size = 0;
	_scale = 0.0f;
}

bool VoxelFont::renderGlyphs(const char* string) {
	static const voxel::Voxel& voxel = voxel::createVoxel(voxel::Grass1);
	const char **s = &string;
	for (int c = core::utf8::next(s); c != -1; c = core::utf8::next(s)) {
		int w;
		int h;
		unsigned char *bitmap = stbtt_GetCodepointBitmap(&_font, 0.0f, _scale, c, &w, &h, 0, 0);
		if (bitmap == nullptr) {
			Log::warn("Could not create mesh for character: %i", c);
			continue;
		}
		voxel::Region region(0, 0, 0, w + 1, h + 1, 1);
		voxel::RawVolume v(region);
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				if (bitmap[y * w + x] == 255) {
					v.setVoxel(glm::ivec3(x, y, 0), voxel);
				}
			}
		}
		stbtt_FreeBitmap(bitmap, nullptr);
		voxel::Mesh* mesh = new voxel::Mesh(8, 8);
		voxel::extractCubicMesh(&v, v.getEnclosingRegion(), mesh, voxel::IsQuadNeeded(true));
		_cache[c] = mesh;
	}
	return true;
}

// TODO: metrics support
void VoxelFont::render(const char* string, std::vector<glm::vec4>& pos, std::vector<uint32_t>& indices) {
	const char **s = &string;
	int x = 0;
	int y = 0;
	int charCount = 0;
	for (int c = core::utf8::next(s); c != -1; c = core::utf8::next(s), ++charCount) {
		if (c == ' ') {
			x += _size;
			continue;
		} else if (c == '\n') {
			x = 0;
			y += _size;
		}
		auto i = _cache.find(c);
		if (i == _cache.end()) {
			x += _size;
			Log::warn("Could not find character glyph cache for %i", c);
			continue;
		}
		const voxel::Mesh* mesh = i->second;
		const voxel::IndexType* meshIndices = mesh->getRawIndexData();
		const voxel::Vertex* meshVertices = mesh->getRawVertexData();
		const size_t meshNumberIndices = mesh->getNoOfIndices();
		const size_t meshNumberVertices = mesh->getNoOfVertices();

		const size_t positionSize = pos.size();
		const size_t indicesSize = indices.size();
		pos.reserve(positionSize + meshNumberVertices);
		indices.reserve(indicesSize + meshNumberIndices);

		for (size_t i = 0; i < meshNumberVertices; ++i) {
			glm::vec4 vp = glm::vec4(meshVertices[i].position, 1.0f);
			vp.x += _size * charCount;
			pos.push_back(vp);
		}
		for (size_t i = 0; i < meshNumberIndices; ++i) {
			// offset by the already added vertices
			indices.push_back(meshIndices[i] + positionSize);
		}
	}
}

}
