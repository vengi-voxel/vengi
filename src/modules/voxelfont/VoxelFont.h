/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <glm/fwd.hpp>
#include <stdint.h>

struct stbtt_fontinfo;

namespace voxel {
class RawVolumeWrapper;
class Voxel;
}

namespace voxelfont {

/**
 * @brief Will take any TTF font and rasterizes into voxels
 */
class VoxelFont {
private:
	stbtt_fontinfo* _font = nullptr;
	uint8_t *_ttfBuffer = nullptr;
	core::String _filename;

	bool renderGlyphs(const char* string);

public:
	~VoxelFont();

	bool init(const char* font);
	void shutdown();

	int renderCharacter(int codepoint, uint8_t size, int thickness, const glm::ivec3 &pos, voxel::RawVolumeWrapper &volume, const voxel::Voxel& voxel);
};

}
