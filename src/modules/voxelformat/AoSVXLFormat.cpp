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
				const int number4byteChunks = v[0];
				const int topColorStart = v[1];
				const int topColorEnd = v[2]; // inclusive

				for (int i = z; i < topColorStart; ++i) {
					volume->setVoxel(x, flipHeight - i, y, air);
				}

				const uint32_t *rgba = (const uint32_t *)(v + sizeof(uint32_t));
				for (z = topColorStart; z <= topColorEnd; ++z) {
					const glm::vec4& color = core::Color::fromRGBA(*rgba);
					const int index = core::Color::getClosestMatch(color, materialColors);
					volume->setVoxel(x, flipHeight - z, y, voxel::createVoxel(voxel::VoxelType::Generic, index));
					++rgba;
				}

				const int lenBottom = topColorEnd - topColorStart + 1;

				// check for end of data marker
				if (number4byteChunks == 0) {
					// infer ACTUAL number of 4-byte chunks from the length of the color data
					v += sizeof(uint32_t) * (lenBottom + 1);
					break;
				}

				// infer the number of bottom colors in next span from chunk length
				const int len_top = (number4byteChunks - 1) - lenBottom;

				// now skip the v pointer past the data to the beginning of the next span
				v += v[0] * sizeof(uint32_t);

				const int bottomColorEnd = v[3]; // aka air start - exclusive
				const int bottomColorStart = bottomColorEnd - len_top;

				for (z = bottomColorStart; z < bottomColorEnd; ++z) {
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
