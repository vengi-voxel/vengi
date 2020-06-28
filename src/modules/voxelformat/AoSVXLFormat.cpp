/**
 * @file
 */

#include "AoSVXLFormat.h"
#include "core/Assert.h"
#include "voxel/MaterialColor.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include <SDL_stdinc.h>
#include <string.h>

namespace voxel {

bool AoSVXLFormat::loadGroups(const io::FilePtr &file, VoxelVolumes &volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("AOS vxl: Could not load file: File doesn't exist");
		return false;
	}

	const int width = 512, height = 512, depth = 64;
	const voxel::Region region(0, 0, 0, width - 1, depth - 1, height - 1);
	const int flipHeight = depth - 1;
	core_assert(region.isValid());
	RawVolume *volume = new RawVolume(region);
	volumes.push_back(VoxelVolume{volume, file->fileName(), true});

	uint8_t *v;
	const int len = file->read((void**) &v);
	if (len <= 0) {
		Log::error("AOS vxl: Failed to load file %s", file->name().c_str());
		return false;
	}

	const uint8_t *base = v;
	constexpr voxel::Voxel air = voxel::createVoxel(voxel::VoxelType::Air, 0);
	const MaterialColorArray& materialColors = getMaterialColors();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			for (int z = 0; z < depth; ++z) {
				volume->setVoxel(x, flipHeight - z, y, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
			int z = 0;
			for (;;) {
				const int number_4byte_chunks = v[0];
				const int top_color_start = v[1];
				const int top_color_end = v[2]; // inclusive

				for (int i = z; i < top_color_start; ++i) {
					volume->setVoxel(x, flipHeight - i, y, air);
				}

				uint32_t *rgba = (uint32_t *)(v + 4);
				for (z = top_color_start; z <= top_color_end; ++z) {
					const glm::vec4& color = core::Color::fromRGBA(*rgba);
					const int index = core::Color::getClosestMatch(color, materialColors);
					volume->setVoxel(x, flipHeight - z, y, voxel::createVoxel(voxel::VoxelType::Generic, index));
					++rgba;
				}

				const int len_bottom = top_color_end - top_color_start + 1;

				// check for end of data marker
				if (number_4byte_chunks == 0) {
					// infer ACTUAL number of 4-byte chunks from the length of the color data
					v += 4 * (len_bottom + 1);
					break;
				}

				// infer the number of bottom colors in next span from chunk length
				const int len_top = (number_4byte_chunks - 1) - len_bottom;

				// now skip the v pointer past the data to the beginning of the next span
				v += v[0] * 4;

				const int bottom_color_end = v[3]; // aka air start - exclusive
				const int bottom_color_start = bottom_color_end - len_top;

				for (z = bottom_color_start; z < bottom_color_end; ++z) {
					const glm::vec4& color = core::Color::fromRGBA(*rgba);
					const int index = core::Color::getClosestMatch(color, materialColors);
					volume->setVoxel(x, flipHeight - z, y, voxel::createVoxel(voxel::VoxelType::Generic, index));
					++rgba;
				}
			}
		}
	}
	core_assert(v - base == len);
	delete[] base;
	return true;
}

bool AoSVXLFormat::saveGroups(const VoxelVolumes &volumes, const io::FilePtr &file) {
	return false;
}

}
