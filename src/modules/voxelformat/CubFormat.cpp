/**
 * @file
 */

#include "CubFormat.h"
#include "io/FileStream.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include "core/ScopedPtr.h"
#include "voxel/MaterialColor.h"

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load cub file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) != true) { \
		Log::error("Could not load vmx file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

size_t CubFormat::loadPalette(const core::String &filename, io::SeekableReadStream& stream, Palette &palette) {
	uint32_t width, depth, height;
	wrap(stream.readUInt32(width))
	wrap(stream.readUInt32(depth))
	wrap(stream.readUInt32(height))

	if (width > 2048 || height > 2048 || depth > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return false;
	}

	for (uint32_t h = 0u; h < height; ++h) {
		for (uint32_t d = 0u; d < depth; ++d) {
			for (uint32_t w = 0u; w < width; ++w) {
				uint8_t r, g, b;
				wrap(stream.readUInt8(r))
				wrap(stream.readUInt8(g))
				wrap(stream.readUInt8(b))
				if (r == 0u && g == 0u && b == 0u) {
					// empty voxel
					continue;
				}
				const uint32_t color = core::Color::getRGBA(r, g, b);
				palette.addColorToPalette(color);
			}
		}
	}
	return palette.colorCount;
}

bool CubFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	uint32_t width, depth, height;
	wrap(stream.readUInt32(width))
	wrap(stream.readUInt32(depth))
	wrap(stream.readUInt32(height))

	if (width > 2048 || height > 2048 || depth > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return false;
	}

	const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", width, height, depth);
		return false;
	}
	RawVolume *volume = new RawVolume(region);
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	sceneGraph.emplace(core::move(node));

	for (uint32_t h = 0u; h < height; ++h) {
		for (uint32_t d = 0u; d < depth; ++d) {
			for (uint32_t w = 0u; w < width; ++w) {
				uint8_t r, g, b;
				wrap(stream.readUInt8(r))
				wrap(stream.readUInt8(g))
				wrap(stream.readUInt8(b))
				if (r == 0u && g == 0u && b == 0u) {
					// empty voxel
					continue;
				}
				const uint32_t color = core::Color::getRGBA(r, g, b);
				const int index = findClosestIndex(color);
				const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
				// we have to flip depth with height for our own coordinate system
				volume->setVoxel((int)w, (int)h, (int)d, voxel);
			}
		}
	}

	return true;
}

#undef wrap

bool CubFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	RawVolume* mergedVolume = merge(sceneGraph);
	core::ScopedPtr<RawVolume> scopedPtr(mergedVolume);

	const voxel::Region& region = mergedVolume->region();
	RawVolume::Sampler sampler(mergedVolume);
	const glm::ivec3& lower = region.getLowerCorner();

	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();

	// we have to flip depth with height for our own coordinate system
	wrapBool(stream.writeUInt32(width))
	wrapBool(stream.writeUInt32(depth))
	wrapBool(stream.writeUInt32(height))

	const voxel::Palette &palette = voxel::getPalette();
	for (uint32_t y = 0u; y < height; ++y) {
		for (uint32_t z = 0u; z < depth; ++z) {
			for (uint32_t x = 0u; x < width; ++x) {
				core_assert_always(sampler.setPosition(lower.x + x, lower.y + y, lower.z + z));
				const voxel::Voxel& voxel = sampler.voxel();
				if (voxel.getMaterial() == VoxelType::Air) {
					wrapBool(stream.writeUInt8(0))
					wrapBool(stream.writeUInt8(0))
					wrapBool(stream.writeUInt8(0))
					continue;
				}

				const glm::u8vec4 &rgba = core::Color::toRGBA(palette.colors[voxel.getColor()]);
				wrapBool(stream.writeUInt8(rgba.r))
				wrapBool(stream.writeUInt8(rgba.g))
				wrapBool(stream.writeUInt8(rgba.b))
			}
		}
	}
	return true;
}

#undef wrap
#undef wrapBool

}
