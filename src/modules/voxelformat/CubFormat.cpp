/**
 * @file
 */

#include "CubFormat.h"
#include "io/FileStream.h"
#include "voxel/MaterialColor.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/Color.h"
#include "core/ScopedPtr.h"

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

bool CubFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, VoxelVolumes& volumes) {
	uint32_t width, depth, height;
	wrap(stream.readUInt32(width))
	wrap(stream.readUInt32(depth))
	wrap(stream.readUInt32(height))

	if (width > 2048 || height > 2048 || depth > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return false;
	}

	const voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", width, height, depth);
		return false;
	}
	RawVolume *volume = new RawVolume(region);
	volumes.push_back(VoxelVolume{volume, filename, true});

	// TODO: support loading own palette

	const MaterialColorArray& materialColors = getMaterialColors();

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
				const glm::vec4& color = core::Color::fromRGBA(r, g, b, 255);
				const int index = core::Color::getClosestMatch(color, materialColors);
				const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
				// we have to flip depth with height for our own coordinate system
				volume->setVoxel(w, h, d, voxel);
			}
		}
	}

	return true;
}

#undef wrap

bool CubFormat::saveGroups(const VoxelVolumes& volumes, const core::String &filename, io::SeekableWriteStream& stream) {
	RawVolume* mergedVolume = merge(volumes);
	core::ScopedPtr<RawVolume> scopedPtr(mergedVolume);

	const voxel::Region& region = mergedVolume->region();
	RawVolume::Sampler sampler(mergedVolume);
	const glm::ivec3& lower = region.getLowerCorner();

	const MaterialColorArray& materialColors = getMaterialColors();

	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();

	// we have to flip depth with height for our own coordinate system
	wrapBool(stream.writeUInt32(width))
	wrapBool(stream.writeUInt32(depth))
	wrapBool(stream.writeUInt32(height))

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
				const glm::vec4& color = materialColors[voxel.getColor()];
				const glm::u8vec4& rgba = core::Color::getRGBAVec(color);
				wrapBool(stream.writeUInt8(rgba.r))
				wrapBool(stream.writeUInt8(rgba.g))
				wrapBool(stream.writeUInt8(rgba.b))
			}
		}
	}
	return true;
}

}
