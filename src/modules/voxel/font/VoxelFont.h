#pragma once

#include "core/Color.h"
#include "voxel/polyvox/Mesh.h"
#include <unordered_map>
#include "stb_truetype.h"

namespace voxel {

/**
 * @brief Will take any TTF font and rasterizes into voxels
 */
class VoxelFont {
private:
	std::unordered_map<uint32_t, voxel::Mesh*> _cache;
	stbtt_fontinfo _font;
	uint8_t *_ttfBuffer = nullptr;
	int _size = 0;
	float _scale = 0.0f;
	int _height = 0;
	int _spaceWidth = 0;
	/**
	 * ascent is the coordinate above the baseline the font extends;
	 */
	int _ascent = 0;
	/**
	 * descent is the coordinate below the baseline the font extends (i.e. it is typically negative)
	 */
	int _descent = 0;

	bool renderGlyphs(const char* string, bool mergeQuads);
public:
	~VoxelFont();

	/**
	 * @param[in] mergeQuads @c false if you want to set e.g. a different color to each voxel. @c true
	 * if you want to keep the index and vertex count as small as possible.
	 */
	bool init(const char* font, int fontSize, bool mergeQuads, const char* glyphs);
	void shutdown();

	int render(const char* string, std::vector<glm::vec4>& pos, std::vector<uint32_t>& indices);
};

}
