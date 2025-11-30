/**
 * @file
 */

#include "SLAB6VoxFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "scenegraph/SceneGraph.h"
#include "palette/Palette.h"
#include "SLABShared.h"

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

namespace voxelformat {

size_t SLAB6VoxFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive,
								   palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return 0;
	}
	uint32_t depth, height, width;
	wrap(stream->readUInt32(width))
	wrap(stream->readUInt32(depth))
	wrap(stream->readUInt32(height))

	if (width > 2048 || height > 2048 || depth > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", width, height, depth);
		return false;
	}

	stream->skip((int64_t)width * height * depth);
	palette.setSize(palette::PaletteMaxColors);
	for (int i = 0; i < palette.colorCount(); ++i) {
		color::RGBA color;
		wrapBool(priv::readRGBScaledColor(*stream, color))
		palette.setColor(i, color);
	}
	return palette.colorCount();
}

bool SLAB6VoxFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
									   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
									   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint32_t depth, height, width;
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

	const int64_t voxelPos = stream->pos();
	stream->skip((int64_t)width * height * depth);
	palette.setSize(palette::PaletteMaxColors);
	for (int i = 0; i < palette.colorCount(); ++i) {
		color::RGBA color;
		wrapBool(priv::readRGBScaledColor(*stream, color))
		palette.setColor(i, color);
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);

	stream->seek(voxelPos);
	const uint8_t emptyColorIndex = (uint8_t)emptyPaletteIndex();
	// TODO: PERF: use volume sampler
	for (uint32_t w = 0u; w < width; ++w) {
		for (uint32_t d = 0u; d < depth; ++d) {
			for (uint32_t h = 0u; h < height; ++h) {
				uint8_t palIdx;
				wrap(stream->readUInt8(palIdx))
				if (palIdx == emptyColorIndex) {
					continue;
				}
				const voxel::Voxel &voxel = voxel::createVoxel(palette, palIdx);
				// we have to flip depth with height for our own coordinate system
				volume->setVoxel(w, (int)height - (int)h - 1, (int)d, voxel);
			}
		}
	}
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));

	return true;
}

bool SLAB6VoxFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);

	const voxel::Region &region = node->region();
	const palette::Palette &palette = node->palette();
	const uint8_t emptyColorIndex = (uint8_t)emptyPaletteIndex();

	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	wrapBool(stream->writeUInt32(dim.x))
	wrapBool(stream->writeUInt32(dim.z))
	wrapBool(stream->writeUInt32(dim.y))

	// we have to flip depth with height for our own coordinate system
	for (int w = region.getLowerX(); w <= region.getUpperX(); ++w) {
		for (int d = region.getLowerZ(); d <= region.getUpperZ(); ++d) {
			for (int h = region.getUpperY(); h >= region.getLowerY(); --h) {
				const voxel::Voxel &voxel = node->volume()->voxel(w, h, d);
				if (voxel::isAir(voxel.getMaterial())) {
					wrapBool(stream->writeUInt8(emptyColorIndex))
				} else {
					core_assert(voxel.getColor() != emptyColorIndex);
					wrapBool(stream->writeUInt8(voxel.getColor()))
				}
			}
		}
	}

	for (int i = 0; i < palette.colorCount(); ++i) {
		const color::RGBA &color = palette.color(i);
		wrapBool(priv::writeRGBScaledColor(*stream, color))
	}
	for (int i = palette.colorCount(); i < palette::PaletteMaxColors; ++i) {
		color::RGBA color(0);
		wrapBool(priv::writeRGBScaledColor(*stream, color))
	}

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
