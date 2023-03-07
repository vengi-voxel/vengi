/**
 * @file
 */

#include "SLAB6VoxFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "voxel/MaterialColor.h"
#include "scenegraph/SceneGraph.h"

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

bool SLAB6VoxFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, voxel::Palette &palette, const LoadContext &ctx) {
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
		uint8_t r, g, b;
		wrap(stream.readUInt8(r))
		wrap(stream.readUInt8(g))
		wrap(stream.readUInt8(b))

		palette.color(i) = core::RGBA(r, g, b);
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	SceneGraphNode node;
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
				const voxel::Voxel& voxel = voxel::createVoxel(palette, palIdx);
				// we have to flip depth with height for our own coordinate system
				volume->setVoxel((int)width - (int)w - 1, (int)height - (int)h - 1, (int)d, voxel);
			}
		}
	}
	node.setName(filename);
	node.setPalette(palette);
	sceneGraph.emplace(core::move(node));

	return true;
}

bool SLAB6VoxFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) {
	const SceneGraph::MergedVolumePalette &merged = sceneGraph.merge();
	if (merged.first == nullptr) {
		Log::error("Failed to merge volumes");
		return false;
	}

	core::ScopedPtr<voxel::RawVolume> scopedPtr(merged.first);
	const voxel::Region &region = merged.first->region();
	const voxel::Palette &palette = merged.second;
	const uint8_t emptyColorReplacement = palette.findReplacement(255);

	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	wrapBool(stream.writeUInt32(dim.x))
	wrapBool(stream.writeUInt32(dim.z))
	wrapBool(stream.writeUInt32(dim.y))

	// we have to flip depth with height for our own coordinate system
	for (int w = region.getUpperX(); w >= region.getLowerX(); --w) {
		for (int d = region.getLowerZ(); d <= region.getUpperZ(); ++d) {
			for (int h = region.getUpperY(); h >= region.getLowerY(); --h) {
				const voxel::Voxel& voxel = merged.first->voxel(w, h, d);
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
		const core::RGBA &rgba = palette.color(i);
		wrapBool(stream.writeUInt8(rgba.r))
		wrapBool(stream.writeUInt8(rgba.g))
		wrapBool(stream.writeUInt8(rgba.b))
	}
	for (int i = palette.colorCount(); i < voxel::PaletteMaxColors; ++i) {;
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
		wrapBool(stream.writeUInt8(0))
	}

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxel
