/**
 * @file
 */

#include "SLAB6VoxFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "scenegraph/SceneGraph.h"

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

bool SLAB6VoxFormat::readColor(io::SeekableReadStream &stream, core::RGBA &color) const {
	uint8_t r, g, b;
	wrap(stream.readUInt8(b))
	wrap(stream.readUInt8(g))
	wrap(stream.readUInt8(r))
	const float rf = ((float)r / 63.0f * 255.0f);
	const float gf = ((float)g / 63.0f * 255.0f);
	const float bf = ((float)b / 63.0f * 255.0f);
	color = core::RGBA((uint8_t)rf, (uint8_t)gf, (uint8_t)bf);
	return true;
}

bool SLAB6VoxFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
									   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
									   const LoadContext &ctx) {
	uint32_t depth, height, width;
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

	const int64_t voxelPos = stream.pos();
	stream.skip((int64_t)width * height * depth);
	palette.setSize(voxel::PaletteMaxColors);
	for (int i = 0; i < palette.colorCount(); ++i) {
		core::RGBA color;
		wrapBool(readColor(stream, color))
		palette.color(i) = color;
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node;
	node.setVolume(volume, true);

	stream.seek(voxelPos);
	for (uint32_t w = 0u; w < width; ++w) {
		for (uint32_t d = 0u; d < depth; ++d) {
			for (uint32_t h = 0u; h < height; ++h) {
				uint8_t palIdx;
				wrap(stream.readUInt8(palIdx))
				if (palIdx == 255) {
					continue;
				}
				const voxel::Voxel &voxel = voxel::createVoxel(palette, palIdx);
				// we have to flip depth with height for our own coordinate system
				volume->setVoxel(w, (int)height - (int)h - 1, (int)d, voxel);
			}
		}
	}
	node.setName(filename);
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));

	return true;
}

bool SLAB6VoxFormat::writeColor(io::SeekableWriteStream &stream, core::RGBA color) const {
	wrapBool(stream.writeUInt8((uint8_t)((float)color.b * 63.0f / 255.0f)))
	wrapBool(stream.writeUInt8((uint8_t)((float)color.g * 63.0f / 255.0f)))
	wrapBool(stream.writeUInt8((uint8_t)((float)color.r * 63.0f / 255.0f)))
	return true;
}

bool SLAB6VoxFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								io::SeekableWriteStream &stream, const SaveContext &ctx) {
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);

	const voxel::Region &region = node->region();
	const voxel::Palette &palette = node->palette();
	const uint8_t emptyColorReplacement = palette.findReplacement(255);

	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	wrapBool(stream.writeUInt32(dim.x))
	wrapBool(stream.writeUInt32(dim.z))
	wrapBool(stream.writeUInt32(dim.y))

	// we have to flip depth with height for our own coordinate system
	for (int w = region.getLowerX(); w <= region.getUpperX(); ++w) {
		for (int d = region.getLowerZ(); d <= region.getUpperZ(); ++d) {
			for (int h = region.getUpperY(); h >= region.getLowerY(); --h) {
				const voxel::Voxel &voxel = node->volume()->voxel(w, h, d);
				if (voxel::isAir(voxel.getMaterial())) {
					wrapBool(stream.writeUInt8(255))
				} else if (voxel.getColor() == 255) {
					wrapBool(stream.writeUInt8(emptyColorReplacement))
				} else {
					wrapBool(stream.writeUInt8(voxel.getColor()))
				}
			}
		}
	}

	for (int i = 0; i < palette.colorCount(); ++i) {
		const core::RGBA &color = palette.color(i);
		wrapBool(writeColor(stream, color))
	}
	for (int i = palette.colorCount(); i < voxel::PaletteMaxColors; ++i) {
		core::RGBA color(0);
		wrapBool(writeColor(stream, color))
	}

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
