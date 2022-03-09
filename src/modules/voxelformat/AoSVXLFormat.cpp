/**
 * @file
 */

#include "AoSVXLFormat.h"
#include "core/Assert.h"
#include "core/StringUtil.h"
#include "core/collection/Map.h"
#include "core/Log.h"
#include "core/Color.h"
#include <SDL_stdinc.h>
#include <string.h>

namespace voxel {

bool AoSVXLFormat::loadGroups(const core::String& filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	const int width = 512, depths = 512, height = 64;
	const voxel::Region region(0, 0, 0, width - 1, height - 1, depths - 1);
	const int flipHeight = height - 1;
	core_assert(region.isValid());
	RawVolume *volume = new RawVolume(region);
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	sceneGraph.emplace(core::move(node));

	const int64_t length = stream.size();
	uint8_t *v = new uint8_t[length];
	if (stream.read(v, length) == -1) {
		Log::error("AOS vxl: Failed to load file %s", filename.c_str());
		return false;
	}

	const uint8_t *base = v;
	// TODO: allow to export the palette/colors
	core::Map<uint32_t, int, 521> paletteMap(32768);
	for (int z = 0; z < depths; ++z) {
		for (int x = 0; x < width; ++x) {
			int y = 0;
			for (;;) {
				const int number4byteChunks = v[0];
				const int topColorStart = v[1];
				const int topColorEnd = v[2]; // inclusive
				int paletteIndex = 1;
				const uint32_t *rgba = (const uint32_t *)(v + sizeof(uint32_t));
				if (topColorStart < 0 || topColorStart >= height) {
					Log::error("depth (top start) exceeds the max allowed value of %i", height);
					return false;
				}
				if (topColorEnd < 0 || topColorEnd >= height) {
					Log::error("depth (top end) exceeds the max allowed value of %i", height);
					return false;
				}
				for (y = topColorStart; y <= topColorEnd; ++y) {
					// TODO: BGRA with A not being alpha - but some shading stuff?
					if (!paletteMap.get(*rgba, paletteIndex)) {
						const glm::vec4& color = core::Color::fromRGBA(*rgba);
						paletteIndex = findClosestIndex(color);
						if (paletteMap.size() < paletteMap.capacity()) {
							paletteMap.put(*rgba, paletteIndex);
						}
					}
					volume->setVoxel(x, flipHeight - y, z, voxel::createVoxel(voxel::VoxelType::Generic, paletteIndex));
					++rgba;
				}
				for (int i = y; i < height; ++i) {
					volume->setVoxel(x, flipHeight - i, z, voxel::createVoxel(voxel::VoxelType::Generic, paletteIndex));
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
				if (bottomColorStart < 0 || bottomColorStart >= height) {
					Log::error("depth (bottom start) exceeds the max allowed value of %i", height);
					return false;
				}
				if (bottomColorEnd < 0 || bottomColorEnd >= height) {
					Log::error("depth (bottom end) exceeds the max allowed value of %i", height);
					return false;
				}

				for (y = bottomColorStart; y < bottomColorEnd; ++y) {
					if (!paletteMap.get(*rgba, paletteIndex)) {
						const glm::vec4& color = core::Color::fromRGBA(*rgba);
						paletteIndex = findClosestIndex(color);
						if (paletteMap.size() < paletteMap.capacity()) {
							paletteMap.put(*rgba, paletteIndex);
						}
					}
					volume->setVoxel(x, flipHeight - y, z, voxel::createVoxel(voxel::VoxelType::Generic, paletteIndex));
					++rgba;
				}
			}
		}
	}
	core_assert(v - base == length);
	delete[] base;
	return true;
}

bool AoSVXLFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	return false;
}

}
