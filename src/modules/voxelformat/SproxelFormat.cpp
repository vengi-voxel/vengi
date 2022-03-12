/**
 * @file
 */

#include "SproxelFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
#include "voxel/MaterialColor.h"

namespace voxel {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load sproxel csv file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",    \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not load sproxel csv file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",    \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

bool SproxelFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	char buf[512];
	wrapBool(stream.readLine(sizeof(buf), buf))

	core::Tokenizer tok(buf, ",");
	if (tok.size() != 3u) {
		Log::error("Invalid size components found - expected x,y,z");
		return false;
	}

	const int32_t x = core::string::toInt(tok.tokens()[0]);
	const int32_t y = core::string::toInt(tok.tokens()[1]);
	const int32_t z = core::string::toInt(tok.tokens()[2]);
	glm::ivec3 size(x, y, z);

	const voxel::Region region(glm::ivec3(0), glm::ivec3(size) - 1);
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	for (int y = size.y - 1; y >= 0; y--) {
		for (int z = 0; z < size.z; z++) {
			for (int x = 0; x < size.x; x++) {
				int r, g, b, a;
				char hex[10];
				if ((stream.read(hex, 9)) == -1) {
					Log::error("Could not load sproxel csv color line");
					delete volume;
					return false;
				}
				hex[sizeof(hex) - 1] = '\0';
				const int n = SDL_sscanf(hex, "#%02X%02X%02X%02X", &r, &g, &b, &a);
				if (n != 4) {
					Log::error("Failed to parse color %i (%s)", n, hex);
					delete volume;
					return false;
				}
				if (a != 0) {
					const uint32_t color = core::Color::getRGBA(r, g, b, a);
					const uint8_t index = findClosestIndex(color);
					const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
					volume->setVoxel(x, y, z, voxel);
				}
				if (x != size.x - 1) {
					stream.skip(1);
				}
			}
			stream.skip(1);
		}
		stream.skip(1);
	}
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	sceneGraph.emplace(core::move(node));
	return true;
}

bool SproxelFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream &stream) {
	RawVolume *mergedVolume = merge(sceneGraph);

	const voxel::Region &region = mergedVolume->region();
	RawVolume::Sampler sampler(mergedVolume);
	const glm::ivec3 &lower = region.getLowerCorner();

	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();
	if (!stream.writeStringFormat(false, "%i,%i,%i\n", width, height, depth)) {
		Log::error("Could not save sproxel csv file");
		delete mergedVolume;
		return false;
	}

	const voxel::Palette& palette = voxel::getPalette();
	for (int y = height - 1; y >= 0; y--) {
		for (int z = 0u; z < depth; ++z) {
			for (int x = 0u; x < width; ++x) {
				core_assert_always(sampler.setPosition(lower.x + x, lower.y + y, lower.z + z));
				const voxel::Voxel &voxel = sampler.voxel();
				if (voxel.getMaterial() == VoxelType::Air) {
					stream.writeString("#00000000", false);
				} else {
					const glm::u8vec4 &rgba = core::Color::toRGBA(palette.colors[voxel.getColor()]);
					stream.writeStringFormat(false, "#%02X%02X%02X%02X", rgba.r, rgba.g, rgba.b, rgba.a);
				}
				if (x != width - 1) {
					stream.writeString(",", false);
				}
			}
			stream.writeString("\n", false);
		}
		stream.writeString("\n", false);
	}
	delete mergedVolume;
	return true;
}

#undef wrap
#undef wrapBool

}; // namespace voxel
