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

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load AoE vxl file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

bool AoSVXLFormat::loadGroups(const core::String& filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	int64_t initial = stream.pos();
	if (!load(filename, stream, sceneGraph, 512, 64, 512)) {
		stream.seek(initial);
		return load(filename, stream, sceneGraph, 512, 256, 512);
	}
	return true;
}

bool AoSVXLFormat::load(const core::String& filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, int width, int height, int depths) {
	const voxel::Region region(0, 0, 0, width - 1, height - 1, depths - 1);
	const int flipHeight = height - 1;
	core_assert(region.isValid());
	RawVolume *volume = new RawVolume(region);
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	sceneGraph.emplace(core::move(node));

	// TODO: allow to export the palette/colors
	core::Map<uint32_t, int, 521> paletteMap(32768);
	for (int z = 0; z < depths; ++z) {
		for (int x = 0; x < width; ++x) {
			int y = 0;
			for (;;) {
				const int64_t cpos = stream.pos();
				uint8_t number4byteChunks;
				wrap(stream.readUInt8(number4byteChunks))
				uint8_t topColorStart;
				wrap(stream.readUInt8(topColorStart))
				uint8_t topColorEnd;
				wrap(stream.readUInt8(topColorEnd))
				int paletteIndex = 1;
				if (stream.skip(1) == -1) {
					Log::error("failed to skip");
					return false;
				}
				if ((int)topColorStart >= height) {
					Log::error("depth (top start %i exceeds the max allowed value of %i", topColorStart, height);
					return false;
				}
				if (topColorEnd >= height) {
					Log::error("depth (top end %i) exceeds the max allowed value of %i", topColorEnd, height);
					return false;
				}
				for (y = topColorStart; y <= topColorEnd; ++y) {
					uint32_t rgba;
					wrap(stream.readUInt32(rgba))
					// TODO: BGRA with A not being alpha - but some shading stuff?
					if (!paletteMap.get(rgba, paletteIndex)) {
						const glm::vec4& color = core::Color::fromRGBA(rgba);
						paletteIndex = findClosestIndex(color);
						if (paletteMap.size() < paletteMap.capacity()) {
							paletteMap.put(rgba, paletteIndex);
						}
					}
					volume->setVoxel(x, flipHeight - y, z, voxel::createVoxel(voxel::VoxelType::Generic, paletteIndex));
				}
				for (int i = y; i < height; ++i) {
					volume->setVoxel(x, flipHeight - i, z, voxel::createVoxel(voxel::VoxelType::Generic, paletteIndex));
				}
				const int lenBottom = topColorEnd - topColorStart + 1;

				// check for end of data marker
				if (number4byteChunks == 0) {
					if (stream.seek(cpos + (int64_t)(sizeof(uint32_t) * (lenBottom + 1))) == -1) {
						Log::error("failed to skip");
						return false;
					}
					break;
				}

				int64_t rgbaPos = stream.pos();
				// infer the number of bottom colors in next span from chunk length
				const int len_top = (number4byteChunks - 1) - lenBottom;

				if (stream.seek(cpos + (int64_t)(number4byteChunks * sizeof(uint32_t))) == -1) {
					Log::error("failed to seek");
					return false;
				}
				uint8_t bottomColorEnd = 0;
				if (stream.skip(3) == -1) {
					Log::error("failed to skip");
					return false;
				}
				wrap(stream.readUInt8(bottomColorEnd))
				if (stream.seek(-4, SEEK_CUR) == -1) {
					Log::error("failed to seek");
					return false;
				}

				// aka air start - exclusive
				const int bottomColorStart = bottomColorEnd - len_top;
				if (bottomColorStart < 0 || bottomColorStart >= height) {
					Log::error("depth (bottom start %i) exceeds the max allowed value of %i", bottomColorStart, height);
					return false;
				}
				if (bottomColorEnd >= height) {
					Log::error("depth (bottom end %i) exceeds the max allowed value of %i", bottomColorEnd, height);
					return false;
				}

				if (stream.seek(rgbaPos) == -1) {
					Log::error("failed to seek");
					return false;
				}
				for (y = bottomColorStart; y < bottomColorEnd; ++y) {
					uint32_t rgba;
					wrap(stream.readUInt32(rgba))
					if (!paletteMap.get(rgba, paletteIndex)) {
						const glm::vec4& color = core::Color::fromRGBA(rgba);
						paletteIndex = findClosestIndex(color);
						if (paletteMap.size() < paletteMap.capacity()) {
							paletteMap.put(rgba, paletteIndex);
						}
					}
					volume->setVoxel(x, flipHeight - y, z, voxel::createVoxel(voxel::VoxelType::Generic, paletteIndex));
				}
				if (stream.seek(cpos + (int64_t)(number4byteChunks * sizeof(uint32_t))) == -1) {
					Log::error("failed to seek");
					return false;
				}
			}
		}
	}
	return true;
}

bool AoSVXLFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	return false;
}

#undef wrap

}
