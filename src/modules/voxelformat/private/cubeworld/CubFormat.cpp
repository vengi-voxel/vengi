/**
 * @file
 */

#include "CubFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "scenegraph/SceneGraph.h"
#include "palette/PaletteLookup.h"
#include "voxel/RawVolume.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load cub file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load cub file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

size_t CubFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return 0;
	}
	uint32_t width, depth, height;
	wrap(stream->readUInt32(width))
	wrap(stream->readUInt32(depth))
	wrap(stream->readUInt32(height))

	if (width > 2048 || height > 2048 || depth > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return 0;
	}

	palette::RGBABuffer colors;
	for (uint32_t h = 0u; h < height; ++h) {
		for (uint32_t d = 0u; d < depth; ++d) {
			for (uint32_t w = 0u; w < width; ++w) {
				uint8_t r, g, b;
				wrap(stream->readUInt8(r))
				wrap(stream->readUInt8(g))
				wrap(stream->readUInt8(b))
				if (r == 0u && g == 0u && b == 0u) {
					// empty voxel
					continue;
				}
				const color::RGBA color = flattenRGB(r, g, b);
				colors.put(color, true);
			}
		}
	}
	return createPalette(colors, palette);
}

bool CubFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	uint32_t width, depth, height;
	wrap(stream->readUInt32(width))
	wrap(stream->readUInt32(depth))
	wrap(stream->readUInt32(height))

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
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	palette::PaletteLookup palLookup(palette);
	voxel::RawVolume::Sampler sampler(volume);
	sampler.setPosition(0, 0, 0);
	// we have to flip depth with height for our own coordinate system
	for (uint32_t h = 0u; h < height; ++h) { // y
		voxel::RawVolume::Sampler sampler2 = sampler;
		for (uint32_t d = 0u; d < depth; ++d) { // z
			voxel::RawVolume::Sampler sampler3 = sampler2;
			for (uint32_t w = 0u; w < width; ++w) { // x
				uint8_t r, g, b;
				wrap(stream->readUInt8(r))
				wrap(stream->readUInt8(g))
				wrap(stream->readUInt8(b))
				if (r == 0u && g == 0u && b == 0u) {
					// empty voxel
					sampler3.movePositiveX();
					continue;
				}
				const color::RGBA color = flattenRGB(r, g, b);
				const int index = palLookup.findClosestIndex(color);
				const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
				sampler3.setVoxel(voxel);
				sampler3.movePositiveX();
			}
			sampler2.movePositiveZ();
		}
		sampler.movePositiveY();
	}
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));
	return true;
}

#undef wrap

bool CubFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);

	const voxel::Region &region = node->region();
	voxel::RawVolume::Sampler sampler(node->volume());
	const glm::ivec3 &lower = region.getLowerCorner();

	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();

	// we have to flip depth with height for our own coordinate system
	wrapBool(stream->writeUInt32(width))
	wrapBool(stream->writeUInt32(depth))
	wrapBool(stream->writeUInt32(height))

	const palette::Palette &palette = node->palette();
	for (uint32_t y = 0u; y < height; ++y) {
		for (uint32_t z = 0u; z < depth; ++z) {
			for (uint32_t x = 0u; x < width; ++x) {
				core_assert_always(sampler.setPosition(lower.x + x, lower.y + y, lower.z + z));
				const voxel::Voxel &voxel = sampler.voxel();
				if (voxel.getMaterial() == voxel::VoxelType::Air) {
					wrapBool(stream->writeUInt8(0))
					wrapBool(stream->writeUInt8(0))
					wrapBool(stream->writeUInt8(0))
					continue;
				}

				color::RGBA rgba = palette.color(voxel.getColor());
				if (rgba.r == 0u && rgba.g == 0u && rgba.b == 0u) {
					rgba = palette.color(palette.findReplacement(voxel.getColor()));
				}
				wrapBool(stream->writeUInt8(rgba.r))
				wrapBool(stream->writeUInt8(rgba.g))
				wrapBool(stream->writeUInt8(rgba.b))
			}
		}
	}
	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
