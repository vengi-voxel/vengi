/**
 * @file
 */

#include "VoxOldFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "voxel/MaterialColor.h"

#define wrap(read) \
	if ((read) != 0) { \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

#define wrapBool(read) \
	if (!(read)) { \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " SDL_FILE ":%i", SDL_LINE); \
		return false; \
	}

namespace voxelformat {

bool VoxOldFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) {
	uint32_t depth, height, width;
	wrap(stream.readUInt32(depth))
	wrap(stream.readUInt32(height))
	wrap(stream.readUInt32(width))

	if (width > 2048 || height > 2048 || depth > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return false;
	}

	const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", width, height, depth);
		return false;
	}
	voxel::RawVolume *volume = new voxel::RawVolume(region);

	const int64_t voxelPos = stream.pos();
	stream.skip((int64_t)width * height * depth);
	_palette.colorCount = voxel::PaletteMaxColors;
	for (int i = 0; i < _palette.colorCount; ++i) {
		uint8_t r, g, b;
		wrap(stream.readUInt8(r))
		wrap(stream.readUInt8(g))
		wrap(stream.readUInt8(b))

		_palette.colors[i] = core::Color::getRGBA(r, g, b);
	}

	stream.seek(voxelPos);
	for (uint32_t h = 0u; h < height; ++h) {
		for (uint32_t d = 0u; d < depth; ++d) {
			for (uint32_t w = 0u; w < width; ++w) {
				uint8_t palIdx;
				wrap(stream.readUInt8(palIdx))
				if (palIdx == 255) {
					continue;
				}
				const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, palIdx);
				// we have to flip depth with height for our own coordinate system
				volume->setVoxel((int)w, (int)h, (int)d, voxel);
			}
		}
	}
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	node.setPalette(_palette);
	sceneGraph.emplace(core::move(node));

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxel
