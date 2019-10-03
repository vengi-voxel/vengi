/**
 * @file
 */

#include "CubFormat.h"
#include "voxel/MaterialColor.h"
#include "core/String.h"
#include "core/Log.h"
#include "core/Color.h"

namespace voxel {

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load cub file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return VoxelVolumes(); \
	}

VoxelVolumes CubFormat::loadGroups(const io::FilePtr& file) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load cub file: File doesn't exist");
		return VoxelVolumes();
	}
	io::FileStream stream(file.get());

	uint32_t width, depth, height;
	wrap(stream.readInt(width))
	wrap(stream.readInt(depth))
	wrap(stream.readInt(height))

	VoxelVolumes volumes;

	RawVolume *volume = new RawVolume(voxel::Region(0, 0, 0, width - 1, height - 1, depth - 1));
	volumes.push_back(VoxelVolume{volume, file->fileName(), true});

	// TODO: support loading own palette

	const MaterialColorArray& materialColors = getMaterialColors();

	for (uint32_t h = 0u; h < height; ++h) {
		for (uint32_t d = 0u; d < depth; ++d) {
			for (uint32_t w = 0u; w < width; ++w) {
				uint8_t r, g, b;
				wrap(stream.readByte(r))
				wrap(stream.readByte(g))
				wrap(stream.readByte(b))
				if (r == 0u && g == 0u && b == 0u) {
					// empty voxel
					continue;
				}
				const glm::vec4& color = core::Color::fromRGBA(r, g, b, 255);
				int index = core::Color::getClosestMatch(color, materialColors);
				voxel::VoxelType voxelType = voxel::VoxelType::Generic;
				if (index == 0) {
					voxelType = voxel::VoxelType::Air;
				}
				const voxel::Voxel& voxel = voxel::createVoxel(voxelType, index);
				// we have to flip depth with height for our own coordinate system
				volume->setVoxel(w, h, d, voxel);
			}
		}
	}

	return volumes;
}

#undef wrap

bool CubFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	io::FileStream stream(file.get());

	RawVolume* mergedVolume = merge(volumes);

	const voxel::Region& region = mergedVolume->region();
	RawVolume::Sampler sampler(mergedVolume);
	const glm::ivec3& lower = region.getLowerCorner();

	const MaterialColorArray& materialColors = getMaterialColors();

	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();

	// we have to flip depth with height for our own coordinate system
	stream.addInt(width);
	stream.addInt(depth);
	stream.addInt(height);

	for (uint32_t y = 0u; y < height; ++y) {
		for (uint32_t z = 0u; z < depth; ++z) {
			for (uint32_t x = 0u; x < width; ++x) {
				sampler.setPosition(lower.x + x, lower.y + y, lower.z + z);
				const voxel::Voxel& voxel = sampler.voxel();
				if (voxel.getMaterial() == VoxelType::Air) {
					stream.addByte(0);
					stream.addByte(0);
					stream.addByte(0);
					continue;
				}
				const glm::vec4& color = materialColors[voxel.getColor()];
				const glm::u8vec4& rgba = core::Color::getRGBAVec(color);
				stream.addByte(rgba.r);
				stream.addByte(rgba.g);
				stream.addByte(rgba.b);
			}
		}
	}
	delete mergedVolume;
	return true;
}

}
