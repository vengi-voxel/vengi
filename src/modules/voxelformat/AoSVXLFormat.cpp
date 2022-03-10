/**
 * @file
 */

#include "AoSVXLFormat.h"
#include "core/Assert.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/Map.h"
#include "core/Log.h"
#include "core/Color.h"
#include "voxel/MaterialColor.h"
#include "voxelutil/VolumeResizer.h"
#include <SDL_stdinc.h>
#include <string.h>

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load AoE vxl file: Not enough data in stream " CORE_STRINGIFY(read)); \
		delete volume; \
		return false; \
	}

bool AoSVXLFormat::loadGroups(const core::String& filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	int64_t initial = stream.pos();

	size_t n = 0;
	glm::ivec3 size(0);

	while (stream.remaining() >= (int64_t)sizeof(Header)) {
		Header header;
		stream.readUInt8(header.len);
		stream.readUInt8(header.colorStartIdx);
		stream.readUInt8(header.colorEndIdx);
		stream.readUInt8(header.airStartIdx);
		if (header.colorEndIdx + 1 > size.y) {
			size.y = header.colorEndIdx + 1;
		}
		if (!header.len) {
			++n;
		}
		const int64_t spanBytes = header.len > 0 ? header.len * (int)sizeof(uint32_t) : (header.colorEndIdx + 2 - header.colorStartIdx) * (int)sizeof(uint32_t);
		stream.skip(spanBytes);
	}
	size.y = 1 << (int)glm::ceil(glm::log2((float)size.y));
	size.x = size.z = (int)glm::sqrt((float)n);

	stream.seek(initial);

	return loadMap(filename, stream, sceneGraph, size.x, size.y, size.z);
}

bool AoSVXLFormat::loadMap(const core::String& filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, int width, int height, int depths) {
	const voxel::Region region(0, 0, 0, width - 1, height - 1, depths - 1);
	const int flipHeight = height - 1;
	core_assert(region.isValid());
	RawVolume *volume = new RawVolume(region);

	// TODO: allow to export the palette/colors
	core::Map<uint32_t, int, 521> paletteMap(32768);
	for (int z = 0; z < depths; ++z) {
		for (int x = 0; x < width; ++x) {
			int y = 0;
			for (;;) {
				const int64_t cpos = stream.pos();
				Header header;
				wrap(stream.readUInt8(header.len))
				wrap(stream.readUInt8(header.colorStartIdx))
				wrap(stream.readUInt8(header.colorEndIdx))
				wrap(stream.readUInt8(header.airStartIdx))
				int paletteIndex = 1;
				if ((int)header.colorStartIdx >= height) {
					Log::error("depth (top start %i exceeds the max allowed value of %i", header.colorStartIdx, height);
					delete volume;
					return false;
				}
				if (header.colorEndIdx >= height) {
					Log::error("depth (top end %i) exceeds the max allowed value of %i", header.colorEndIdx, height);
					delete volume;
					return false;
				}
				for (y = header.colorStartIdx; y <= header.colorEndIdx; ++y) {
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
				const int lenBottom = header.colorEndIdx - header.colorStartIdx + 1;

				// check for end of data marker
				if (header.len == 0) {
					if (stream.seek(cpos + (int64_t)(sizeof(uint32_t) * (lenBottom + 1))) == -1) {
						Log::error("failed to skip");
						delete volume;
						return false;
					}
					break;
				}

				int64_t rgbaPos = stream.pos();
				// infer the number of bottom colors in next span from chunk length
				const int len_top = (header.len - 1) - lenBottom;

				if (stream.seek(cpos + (int64_t)(header.len * sizeof(uint32_t))) == -1) {
					Log::error("failed to seek");
					delete volume;
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
					delete volume;
					return false;
				}

				// aka air start - exclusive
				const int bottomColorStart = bottomColorEnd - len_top;
				if (bottomColorStart < 0 || bottomColorStart >= height) {
					Log::error("depth (bottom start %i) exceeds the max allowed value of %i", bottomColorStart, height);
					delete volume;
					return false;
				}
				if (bottomColorEnd >= height) {
					Log::error("depth (bottom end %i) exceeds the max allowed value of %i", bottomColorEnd, height);
					delete volume;
					return false;
				}

				if (stream.seek(rgbaPos) == -1) {
					Log::error("failed to seek");
					delete volume;
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
				if (stream.seek(cpos + (int64_t)(header.len * sizeof(uint32_t))) == -1) {
					Log::error("failed to seek");
					delete volume;
					return false;
				}
			}
		}
	}
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	sceneGraph.emplace(core::move(node));
	return true;
}

bool AoSVXLFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	return false;
}

#undef wrap

}
