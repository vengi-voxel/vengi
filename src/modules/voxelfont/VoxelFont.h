/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "image/FontAtlas.h"
#include "math/Axis.h"
#include <stdint.h>
#include "math/Vec3i.h"

namespace voxel {
class RawVolumeWrapper;
class Voxel;
} // namespace voxel

namespace voxelfont {

/**
 * @brief Will take any TTF font and rasterizes into voxels
 */
class VoxelFont {
private:
	image::FontAtlas _atlas;
	core::String _filename;

public:
	~VoxelFont();

	bool init(const core::String &font);
	void shutdown();

	void dimensions(const char *string, uint8_t size, int &w, int &h) const;
	int renderCharacter(int codepoint, uint8_t size, int thickness, const math::Vec3i &pos,
						voxel::RawVolumeWrapper &volume, const voxel::Voxel &voxel, math::Axis axis = math::Axis::X);
};

} // namespace voxelfont
