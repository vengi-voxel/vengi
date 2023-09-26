/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"
#include "core/collection/Buffer.h"

namespace io {
class ZipReadStream;
}

namespace voxelformat {

namespace priv {
class NamedBinaryTag;
}

/**
 * @note https://minecraft.wiki/w/Schematic_file_format
 * @note https://github.com/SpongePowered/Schematic-Specification/tree/master/versions
 *
 * @ingroup Formats
 */
class SchematicFormat : public PaletteFormat {
protected:
	bool loadSponge1And2(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
						 voxel::Palette &palette);
	bool parseBlockData(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
						voxel::Palette &palette, const priv::NamedBinaryTag &blocks);

	bool loadNbt(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
				 int dataVersion);

	bool loadSponge3(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
					 int version);
	bool parseBlocks(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
					 const priv::NamedBinaryTag &blocks, int version);

	void addMetadata_r(const core::String &key, const priv::NamedBinaryTag &schematic,
					   scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node);
	void parseMetadata(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
					   scenegraph::SceneGraphNode &node);
	int parsePalette(const priv::NamedBinaryTag &schematic, core::Buffer<int> &mcpal) const;
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;
};

} // namespace voxelformat
