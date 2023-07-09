/**
 * @file
 */

#include "XRawFormat.h"
#include "core/Color.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/FileStream.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/PaletteLookup.h"
#include "voxel/RawVolume.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load xraw file: Not enough data in stream " CORE_STRINGIFY(read));                       \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load xraw file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",           \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

enum class ColorChannelDataType : uint8_t { TypeUnsignedInteger = 0, TypeSignedInteger = 1, TypeFloat = 2 };

enum class ColorChannelCount : uint8_t {
	RGBA = 4, //  R is stored first
	RGB = 3,
	RG = 2,
	R = 1
};

/**
 * @param[in] bitsPerColorChannel 8, 16, 32
 */
static core::RGBA readColor(io::SeekableReadStream &stream) {
	core::RGBA rgba;
	stream.readUInt32(rgba.rgba);
	return rgba;
}

/**
 * @param[in] bitsPerIndex
 *  8: 256 colors: 0 for empty voxel
 *  16: 32768 colors: ~0 for empty voxel
 *  0: no palette
 */
static int readVoxel(io::SeekableReadStream &stream, const voxel::Palette &palette, uint32_t paletteSize,
					 uint8_t bitsPerIndex) {
	if (paletteSize == 0 || bitsPerIndex == 0u) {
		const core::RGBA rgba = readColor(stream);
		return palette.getClosestMatch(rgba);
	}

	if (bitsPerIndex == 8) {
		uint8_t index;
		wrap(stream.readUInt8(index))
		return index;
	} else if (bitsPerIndex == 16) {
		uint16_t index;
		wrap(stream.readUInt16(index))
		return index;
	} else {
		Log::error("Could not load xraw file: Invalid bits per index: %i", bitsPerIndex);
		return -1;
	}

	return 0;
}

size_t XRawFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
							   const LoadContext &ctx) {
	uint32_t magic;
	wrap(stream.readUInt32(magic))

	if (magic != FourCC('X', 'R', 'A', 'W')) {
		Log::error("Could not load xraw file: Invalid magic number");
		return 0;
	}

	ColorChannelDataType colorChannelDataType;
	wrap(stream.readUInt8((uint8_t &)colorChannelDataType))

	ColorChannelCount colorChannelCount;
	wrap(stream.readUInt8((uint8_t &)colorChannelCount))

	uint8_t bitsPerColorChannel;
	wrap(stream.readUInt8(bitsPerColorChannel))

	uint8_t bitsPerIndex;
	wrap(stream.readUInt8(bitsPerIndex))

	// address = x + y * width + z * (width * height)
	uint32_t width, depth, height;
	wrap(stream.readUInt32(width))
	wrap(stream.readUInt32(depth))
	wrap(stream.readUInt32(height))

	if (width > 2048 || height > 2048 || depth > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return 0;
	}

	// 256, 32768
	uint32_t paletteSize;
	wrap(stream.readUInt32(paletteSize))

	if (colorChannelDataType != ColorChannelDataType::TypeUnsignedInteger) {
		Log::error("Could not load xraw file: Unsupported color channel data type: %i", (int)colorChannelDataType);
		return 0;
	}
	if (colorChannelCount != ColorChannelCount::RGBA) {
		Log::error("Could not load xraw file: Unsupported color channel count: %i", (int)colorChannelCount);
		return 0;
	}
	if (bitsPerColorChannel != 8) {
		Log::error("Could not load xraw file: Unsupported bits per color channel: %i", bitsPerColorChannel);
		return 0;
	}

	// end of header

	// voxel buffer, voxels if no palette, indices if palette
	if (paletteSize == 0 || bitsPerIndex == 0u) {
		Log::debug("No palette found - not supported yet to build on with the rgba values of the voxels");
		return 0;
	}

	if (stream.skip(width * height * depth * bitsPerIndex / 8u) == -1) {
		Log::error("Could not load xraw file: Not enough data in stream");
		return 0;
	}

	// palette buffer

	for (uint32_t i = 0u; i < paletteSize; ++i) {
		const core::RGBA rgba = readColor(stream);
		const core::RGBA color = flattenRGB(rgba);
		palette.addColorToPalette(color, false);
	}
	// end of file
	return palette.size();
}

bool XRawFormat::loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
								scenegraph::SceneGraph &sceneGraph, const voxel::Palette &palette,
								const LoadContext &ctx) {
	uint32_t magic;
	wrap(stream.readUInt32(magic))

	if (magic != FourCC('X', 'R', 'A', 'W')) {
		Log::error("Could not load xraw file: Invalid magic number");
		return false;
	}

	ColorChannelDataType colorChannelDataType;
	wrap(stream.readUInt8((uint8_t &)colorChannelDataType))

	ColorChannelCount colorChannelCount;
	wrap(stream.readUInt8((uint8_t &)colorChannelCount))

	uint8_t bitsPerColorChannel;
	wrap(stream.readUInt8(bitsPerColorChannel))

	uint8_t bitsPerIndex;
	wrap(stream.readUInt8(bitsPerIndex))

	// address = x + y * width + z * (width * height)
	uint32_t width, depth, height;
	wrap(stream.readUInt32(width))
	wrap(stream.readUInt32(depth))
	wrap(stream.readUInt32(height))

	if (width > 2048 || height > 2048 || depth > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return false;
	}

	// 256, 32768
	uint32_t paletteSize;
	wrap(stream.readUInt32(paletteSize))

	// end of header

	const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", width, height, depth);
		return false;
	}
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	for (uint32_t h = 0u; h < height; ++h) {
		for (uint32_t d = 0u; d < depth; ++d) {
			for (uint32_t w = 0u; w < width; ++w) {
				const int index = readVoxel(stream, palette, paletteSize, bitsPerIndex);
				if (index == 0 || index == ~(uint16_t)0) {
					continue;
				}
				const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
				// we have to flip depth with height for our own coordinate system
				volume->setVoxel((int)w, (int)h, (int)d, voxel);
			}
		}
	}
	scenegraph::SceneGraphNode node;
	node.setVolume(volume, true);
	node.setName(filename);
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));
	return true;
}

#undef wrap

bool XRawFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							io::SeekableWriteStream &stream, const SaveContext &ctx) {
	const scenegraph::SceneGraph::MergedVolumePalette &merged = sceneGraph.merge();
	if (merged.first == nullptr) {
		Log::error("Failed to merge volumes");
		return false;
	}
	core::ScopedPtr<voxel::RawVolume> scopedPtr(merged.first);

	const voxel::Region &region = merged.first->region();
	voxel::RawVolume::Sampler sampler(merged.first);
	const glm::ivec3 &lower = region.getLowerCorner();

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
				const voxel::Voxel &voxel = sampler.voxel();
				if (voxel.getMaterial() == voxel::VoxelType::Air) {
					wrapBool(stream.writeUInt8(0))
					wrapBool(stream.writeUInt8(0))
					wrapBool(stream.writeUInt8(0))
					continue;
				}

				core::RGBA rgba = palette.color(voxel.getColor());
				if (rgba.r == 0u && rgba.g == 0u && rgba.b == 0u) {
					rgba = palette.color(palette.findReplacement(voxel.getColor()));
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

} // namespace voxelformat
