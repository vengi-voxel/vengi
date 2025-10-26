/**
 * @file
 */

#include "XRawFormat.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MaterialColor.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"

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

static core::RGBA readColor(io::SeekableReadStream &stream) {
	core::RGBA rgba;
	if (stream.readUInt32(rgba.rgba) == -1) {
		Log::error("Could not load xraw palette data: Not enough data in stream");
		return core::RGBA(0);
	}
	return rgba;
}

/**
 * @param[in] bitsPerIndex
 *  8: 256 colors: 0 for empty voxel
 *  16: 32768 colors: ~0 for empty voxel
 *  0: no palette
 */
static int readVoxel(io::SeekableReadStream &stream, const palette::Palette &palette, uint32_t paletteSize,
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
	}

	Log::error("Could not load xraw file: Invalid bits per index: %i", bitsPerIndex);
	return -1;
}

size_t XRawFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return 0;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))
	Log::debug("Try loading xraw palette from file %s", filename.c_str());

	if (magic != FourCC('X', 'R', 'A', 'W')) {
		Log::error("Could not load xraw file: Invalid magic number");
		return 0;
	}

	ColorChannelDataType colorChannelDataType;
	wrap(stream->readUInt8((uint8_t &)colorChannelDataType))
	Log::debug("Color channel data type: %i", (int)colorChannelDataType);

	ColorChannelCount colorChannelCount;
	wrap(stream->readUInt8((uint8_t &)colorChannelCount))
	Log::debug("Color channel count: %i", (int)colorChannelCount);

	uint8_t bitsPerColorChannel;
	wrap(stream->readUInt8(bitsPerColorChannel))
	Log::debug("Bits per color channel: %u", bitsPerColorChannel);

	uint8_t bitsPerIndex;
	wrap(stream->readUInt8(bitsPerIndex))
	Log::debug("Bits per index: %u", bitsPerIndex);

	// address = x + y * width + z * (width * height)
	uint32_t width, depth, height;
	wrap(stream->readUInt32(width))
	wrap(stream->readUInt32(depth))
	wrap(stream->readUInt32(height))
	Log::debug("Width: %u, Depth: %u, Height: %u", width, depth, height);

	if (width > 2048 || height > 2048 || depth > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return 0;
	}

	// 256, 32768
	uint32_t paletteSize;
	wrap(stream->readUInt32(paletteSize))
	Log::debug("Palette size: %u", paletteSize);

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
		Log::debug("No palette found - not supported yet to handle rgba values of the voxels");
		return 0;
	}

	if (stream->skip(width * height * depth * bitsPerIndex / 8u) == -1) {
		Log::error("Could not load xraw file: Not enough data in stream");
		return 0;
	}

	// palette buffer
	if (paletteSize <= (uint32_t)palette::PaletteMaxColors) {
		Log::debug("Loading palette with %u colors", paletteSize);
		for (uint32_t i = 0u; i < paletteSize; ++i) {
			const core::RGBA rgba = readColor(*stream);
			palette.setColor(i, rgba);
		}
	} else {
		// we have to create a palette from the colors
		Log::debug("Palette size exceeds the max allowed size: %i (we have to quantize the colors)", paletteSize);
		palette::RGBABuffer colors;
		for (uint32_t i = 0u; i < paletteSize; ++i) {
			const core::RGBA rgba = flattenRGB(readColor(*stream));
			colors.put(rgba, true);
		}
		createPalette(colors, palette);
	}
	// end of file
	return palette.size();
}

bool XRawFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
								scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))

	if (magic != FourCC('X', 'R', 'A', 'W')) {
		Log::error("Could not load xraw file: Invalid magic number");
		return false;
	}

	ColorChannelDataType colorChannelDataType;
	wrap(stream->readUInt8((uint8_t &)colorChannelDataType))
	Log::debug("Color channel data type: %i", (int)colorChannelDataType);
	if (colorChannelDataType != ColorChannelDataType::TypeUnsignedInteger) {
		Log::error("Could not load xraw file: Unsupported color channel data type: %i", (int)colorChannelDataType);
		return 0;
	}

	ColorChannelCount colorChannelCount;
	wrap(stream->readUInt8((uint8_t &)colorChannelCount))
	Log::debug("Color channel count: %i", (int)colorChannelCount);
	if (colorChannelCount != ColorChannelCount::RGBA) {
		Log::error("Could not load xraw file: Unsupported color channel count: %i", (int)colorChannelCount);
		return 0;
	}

	uint8_t bitsPerColorChannel;
	wrap(stream->readUInt8(bitsPerColorChannel))
	Log::debug("Bits per color channel: %u", bitsPerColorChannel);
	if (bitsPerColorChannel != 8) {
		Log::error("Could not load xraw file: Unsupported bits per color channel: %i", bitsPerColorChannel);
		return 0;
	}

	uint8_t bitsPerIndex;
	wrap(stream->readUInt8(bitsPerIndex))
	Log::debug("Bits per index: %u", bitsPerIndex);

	// address = x + y * width + z * (width * height)
	uint32_t width, depth, height;
	wrap(stream->readUInt32(width))
	wrap(stream->readUInt32(depth))
	wrap(stream->readUInt32(height))
	Log::debug("Width: %u, Depth: %u, Height: %u", width, depth, height);

	if (width > 2048 || height > 2048 || depth > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return false;
	}

	// 256, 32768
	uint32_t paletteSize;
	wrap(stream->readUInt32(paletteSize))
	Log::debug("Palette size: %u", paletteSize);

	// end of header

	const voxel::Region region(0, 0, 0, (int)width - 1, (int)height - 1, (int)depth - 1);
	if (!region.isValid()) {
		Log::error("Invalid region: %i:%i:%i", width, height, depth);
		return false;
	}
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	voxel::RawVolume::Sampler sampler(volume);
	sampler.setPosition(width - 1, 0, 0);
	for (uint32_t h = 0u; h < height; ++h) {
		voxel::RawVolume::Sampler sampler2 = sampler;
		for (uint32_t d = 0u; d < depth; ++d) {
			voxel::RawVolume::Sampler sampler3 = sampler2;
			for (uint32_t w = 0u; w < width; ++w) {
				const int index = readVoxel(*stream, palette, paletteSize, bitsPerIndex);
				if (index == 0 || index == ~(uint16_t)0) {
					sampler3.moveNegativeX();
					continue;
				}
				const voxel::Voxel &voxel = voxel::createVoxel(palette, index);
				// we have to flip depth with height for our own coordinate system
				sampler3.setVoxel(voxel);
				sampler3.moveNegativeX();
			}
			sampler2.movePositiveZ();
		}
		sampler.movePositiveY();
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

#undef wrap

bool XRawFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);
	const voxel::Region &region = node->region();

	wrapBool(stream->writeUInt32(FourCC('X', 'R', 'A', 'W')))

	wrapBool(stream->writeUInt8((uint8_t)ColorChannelDataType::TypeUnsignedInteger))
	wrapBool(stream->writeUInt8((uint8_t)ColorChannelCount::RGBA))
	wrapBool(stream->writeUInt8(8)) // bitsPerColorChannel
	wrapBool(stream->writeUInt8(8)) // bitsPerIndex

	// we have to flip depth with height for our own coordinate system
	wrapBool(stream->writeUInt32(region.getWidthInVoxels()))
	wrapBool(stream->writeUInt32(region.getDepthInVoxels()))
	wrapBool(stream->writeUInt32(region.getHeightInVoxels()))

	wrapBool(stream->writeUInt32(palette::PaletteMaxColors))

	palette::Palette palette = node->palette();
	uint8_t replacement = palette.findReplacement(emptyPaletteIndex());
	const core::RGBA color = palette.color(emptyPaletteIndex());
	if (color.a != 0 && palette.colorCount() < palette::PaletteMaxColors) {
		palette.setColor(0, core::RGBA(0, 0, 0, 0));
		palette.tryAdd(color, false, &replacement, false, 0);
	}
	auto func = [&stream, replacement](int, int, int, const voxel::Voxel &voxel) {
		if (voxel.getMaterial() == voxel::VoxelType::Air) {
			stream->writeUInt8(0);
		} else if (voxel.getColor() == 0) {
			stream->writeUInt8(replacement);
		} else {
			stream->writeUInt8(voxel.getColor());
		}
	};
	voxelutil::visitVolume(*node->volume(), func, voxelutil::VisitAll(), voxelutil::VisitorOrder::YZmX);

	wrapBool(stream->writeUInt32(0)) // first palette entry is always 0 - empty voxel
	for (int i = 1; i < palette.colorCount(); ++i) {
		const core::RGBA rgba = palette.color(i);
		wrapBool(stream->writeUInt32(rgba.rgba))
	}
	for (int i = palette.colorCount(); i < palette::PaletteMaxColors; ++i) {
		wrapBool(stream->writeUInt32(0))
	}

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
