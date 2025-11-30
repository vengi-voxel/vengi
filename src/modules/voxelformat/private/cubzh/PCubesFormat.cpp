/**
 * @file
 */

#include "PCubesFormat.h"
#include "CubzhShared.h"
#include "core/ScopedPtr.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::error("Could not save pcubes file: Not enough data in stream " CORE_STRINGIFY(read));                     \
		return false;                                                                                                  \
	}

bool PCubesFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							  const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}

	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();

	stream->write("PARTICUBES!", 11);
	wrapBool(stream->writeUInt32(6)) // version
	wrapBool(stream->writeUInt8(1))	// zip compression
	const int64_t totalSizePos = stream->pos();
	wrapBool(stream->writeUInt32(0)) // total size is written at the end
	const int64_t afterHeaderPos = stream->pos();

	{
		WriteChunkStream sub(priv::CHUNK_ID_PALETTE_LEGACY_V6, *stream);
		const palette::Palette &palette = node->palette();
		const uint8_t colorCount = palette.colorCount();
		wrapBool(sub.writeUInt8(1))
		wrapBool(sub.writeUInt8(colorCount))
		wrapBool(sub.writeUInt16(colorCount))
		wrapBool(sub.writeUInt8(0)) // default color
		wrapBool(sub.writeUInt8(0)) // default background color
		for (uint8_t i = 0; i < colorCount; ++i) {
			const color::RGBA rgba = palette.color(i);
			wrapBool(sub.writeUInt8(rgba.r))
			wrapBool(sub.writeUInt8(rgba.g))
			wrapBool(sub.writeUInt8(rgba.b))
			wrapBool(sub.writeUInt8(rgba.a))
		}
		for (uint8_t i = 0; i < colorCount; ++i) {
			wrapBool(sub.writeBool(palette.hasEmit(i)))
		}
	}
	{
		WriteChunkStream ws(priv::CHUNK_ID_SHAPE_V6, *stream);
		{
			WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_SIZE_V6, ws);
			const glm::ivec3 &dimensions = node->region().getDimensionsInVoxels();
			wrapBool(sub.writeUInt16(dimensions.x))
			wrapBool(sub.writeUInt16(dimensions.y))
			wrapBool(sub.writeUInt16(dimensions.z))
		}
		{
			WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_PIVOT_V6, ws);
			const glm::vec3 &pivot = node->worldPivot();
			wrapBool(sub.writeFloat(pivot.x))
			wrapBool(sub.writeFloat(pivot.y))
			wrapBool(sub.writeFloat(pivot.z))
		}
		{
			WriteSubChunkStream sub(priv::CHUNK_ID_SHAPE_BLOCKS_V6, ws);
			const voxel::RawVolume *volume = node->volume();
			const voxel::Region &region = volume->region();
			const uint8_t emptyColorIndex = (uint8_t)emptyPaletteIndex();
			for (int x = region.getUpperX(); x >= region.getLowerX(); x--) {
				for (int y = region.getLowerY(); y <= region.getUpperY(); y++) {
					for (int z = region.getLowerZ(); z <= region.getUpperZ(); z++) {
						const voxel::Voxel &voxel = volume->voxel(x, y, z);
						if (voxel::isAir(voxel.getMaterial())) {
							wrapBool(sub.writeUInt8(emptyColorIndex))
						} else {
							wrapBool(sub.writeUInt8(voxel.getColor()))
						}
					}
				}
			}
		}
	}

	const uint32_t totalSize = (uint32_t)(stream->size() - afterHeaderPos);
	if (stream->seek(totalSizePos) == -1) {
		Log::error("Failed to seek to the total size position in the header");
		return false;
	}
	wrapBool(stream->writeUInt32(totalSize))
	stream->seek(0, SEEK_END);
	return true;
}

#undef wrapBool

} // namespace voxelformat
