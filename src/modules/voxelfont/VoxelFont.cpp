/**
 * @file
 */

#include "VoxelFont.h"
#include "core/Common.h"
#include "core/Log.h"
#include "voxel/RawVolumeWrapper.h"

namespace voxelfont {

VoxelFont::~VoxelFont() {
	shutdown();
}

bool VoxelFont::init(const core::String &filename) {
	if (_filename == filename) {
		return true;
	}
	shutdown();
	if (!_atlas.init(filename.c_str())) {
		Log::info("Failed to initialize voxel font, %s doesn't exist", filename.c_str());
		return false;
	}

	Log::info("Initialized voxel font for %s", filename.c_str());
	_filename = filename;
	return true;
}

void VoxelFont::shutdown() {
	_atlas.shutdown();
	_filename = "";
}

void VoxelFont::dimensions(const char *string, uint8_t size, int &w, int &h) const {
	_atlas.measure(string, (float)size, w, h);
}

int VoxelFont::renderCharacter(int codepoint, uint8_t size, int thickness, const glm::ivec3 &pos,
							   voxel::RawVolumeWrapper &volume, const voxel::Voxel &voxel, math::Axis axis) {
	uint8_t *bitmap = nullptr;
	int w = 0;
	int h = 0;
	int ix0 = 0;
	int iy0 = 0;
	if (!_atlas.rasterize((uint32_t)codepoint, (float)size, bitmap, w, h, ix0, iy0)) {
		Log::warn("Could not create voxelfont mesh for character: %i", codepoint);
		return 0;
	}
	thickness = core_max(1, thickness);

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
	_atlas.freeBitmap(bitmap);
	return w;
}

} // namespace voxelfont
