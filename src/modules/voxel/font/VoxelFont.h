#pragma once

#include "core/Color.h"
#include "voxel/polyvox/RawVolume.h"
#include <unordered_map>
#include "stb_truetype.h"

namespace voxel {

class VoxelFont {
private:
	struct CharCache {
		uint8_t *buffer;
		size_t bufferSize;
	};
	std::unordered_map<uint32_t, CharCache> _cache;
	stbtt_fontinfo _font;
public:
	bool renderString(const char* string, int size, const glm::vec4& color, uint8_t *buffer, size_t bufferSize);

	~VoxelFont();
};

}
