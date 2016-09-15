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
	int _height = 0;
	/**
	 * ascent is the coordinate above the baseline the font extends;
	 */
	int _ascent = 0;
	/**
	 * descent is the coordinate below the baseline the font extends (i.e. it is typically negative)
	 */
	int _descent = 0;

	bool renderGlyphs(const char* string);
public:
	~VoxelFont();

	bool init(const char* font, int fontSize, const char* glyphs);
	void shutdown();

	int render(const char* string, std::vector<glm::vec4>& pos, std::vector<uint32_t>& indices);
};

}
