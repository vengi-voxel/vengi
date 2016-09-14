#pragma once

#include "core/Color.h"
#include "voxel/polyvox/Mesh.h"
#include <unordered_map>
#include "stb_truetype.h"

namespace frontend {

class VoxelFont {
private:
	std::unordered_map<uint32_t, voxel::Mesh*> _cache;
	stbtt_fontinfo _font;
	uint8_t *_ttfBuffer = nullptr;
	int _size = 0;
	float _scale = 0.0f;

	bool renderGlyphs(const char* string);
public:
	bool init(const char* font, int fontSize, const char* glyphs);
	void shutdown();

	~VoxelFont();
};

}
