/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief anivoxel format
 *
 * @ingroup Formats
 */
class AniVoxelFormat : public PaletteFormat {
protected:
	struct ChunkHeader {
		uint32_t id;
		uint32_t offset;
		uint32_t size;
		uint32_t position;
	};
	ChunkHeader readChunk(io::SeekableReadStream &stream);
	bool skipChunk(io::SeekableReadStream &stream);
	void seek(const ChunkHeader &header, io::SeekableReadStream &stream);
	bool readArmature(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx,
					  int version);
	bool readAnimation(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx,
					   int version);

	bool readBuffers(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx);
	bool readBone(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx);
	bool readPalette(io::SeekableReadStream &stream, palette::Palette &palette, const LoadContext &ctx);
	bool readMaterial(io::SeekableReadStream &stream, palette::Palette &palette);
	bool readModel(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
				   const LoadContext &ctx, int version);

	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"anivoxel", {"voxa"}, {"VOXA"}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED /* | FORMAT_FLAG_SAVE */};
		return f;
	}
};

} // namespace voxelformat
