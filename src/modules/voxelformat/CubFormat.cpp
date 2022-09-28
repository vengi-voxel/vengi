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
#include "voxel/PaletteLookup.h"

namespace voxelformat {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load cub file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) != true) { \
		Log::error("Could not load cub file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

size_t CubFormat::loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) {
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
				const core::RGBA color = core::Color::getRGBA(r, g, b);
				palette.addColorToPalette(color, false);
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
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	voxel::PaletteLookup palLookup;
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
				const core::RGBA color = core::Color::getRGBA(r, g, b);
				const int index = palLookup.findClosestIndex(color);
				const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
				// we have to flip depth with height for our own coordinate system
				volume->setVoxel((int)w, (int)h, (int)d, voxel);
			}
		}
	}
	SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	node.setPalette(palLookup.palette());
	sceneGraph.emplace(core::move(node));
	sceneGraph.updateTransforms();
	return true;
}

#undef wrap

bool CubFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	const SceneGraph::MergedVolumePalette &merged = sceneGraph.merge();
	if (merged.first == nullptr) {
		Log::error("Failed to merge volumes");
		return false;
	}
	core::ScopedPtr<voxel::RawVolume> scopedPtr(merged.first);

	const voxel::Region& region = merged.first->region();
	voxel::RawVolume::Sampler sampler(merged.first);
	const glm::ivec3& lower = region.getLowerCorner();

	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();

	// we have to flip depth with height for our own coordinate system
	wrapBool(stream.writeUInt32(width))
	wrapBool(stream.writeUInt32(depth))
	wrapBool(stream.writeUInt32(height))

	const voxel::Palette &palette = merged.second;
	for (uint32_t y = 0u; y < height; ++y) {
		for (uint32_t z = 0u; z < depth; ++z) {
			for (uint32_t x = 0u; x < width; ++x) {
				core_assert_always(sampler.setPosition(lower.x + x, lower.y + y, lower.z + z));
				const voxel::Voxel& voxel = sampler.voxel();
				if (voxel.getMaterial() == voxel::VoxelType::Air) {
					wrapBool(stream.writeUInt8(0))
					wrapBool(stream.writeUInt8(0))
					wrapBool(stream.writeUInt8(0))
					continue;
				}

				core::RGBA rgba = palette.colors[voxel.getColor()];
				if (rgba.r == 0u && rgba.g == 0u && rgba.b == 0u) {
					rgba = palette.colors[palette.findReplacement(voxel.getColor())];
				}
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
