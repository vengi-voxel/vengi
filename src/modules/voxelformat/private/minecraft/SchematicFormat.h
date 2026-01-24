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
 * @note https://www.minecraft-schematics.com/
 * @note https://github.com/SpongePowered/Schematic-Specification/tree/master/versions
 * @note https://abfielder.com/
 * @note Details about the bp format are from https://github.com/PiTheGuy/SchemConvert
 *
 * @ingroup Formats
 */
class SchematicFormat : public PaletteFormat {
protected:
	using SchematicPalette = core::Buffer<int>;
	bool loadSponge1And2(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
						 palette::Palette &palette);
	bool parseBlockData(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
						palette::Palette &palette, const priv::NamedBinaryTag &blocks);

	bool loadNbt(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
				 int dataVersion);
	bool readLitematicBlockStates(const glm::ivec3 &size, int bits, const priv::NamedBinaryTag &blockStates,
								  scenegraph::SceneGraphNode &node, const SchematicPalette &mcpal);
	bool loadLitematic(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
					   palette::Palette &palette);
	bool loadAxiomBinary(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, palette::Palette &palette);
	bool loadAxiom(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
				  palette::Palette &palette);
	bool loadSponge3(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
					 palette::Palette &palette, int version);
	bool parseBlocks(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
					 const priv::NamedBinaryTag &blocks, int version);

	void addMetadata_r(const core::String &key, const priv::NamedBinaryTag &schematic,
					   scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node);
	void parseMetadata(const priv::NamedBinaryTag &schematic, scenegraph::SceneGraph &sceneGraph,
					   scenegraph::SceneGraphNode &node);
	int loadMCEdit2Palette(const priv::NamedBinaryTag &schematic, SchematicPalette &mcpal) const;
	int loadWorldEditPalette(const priv::NamedBinaryTag &schematic, SchematicPalette &mcpal) const;
	int loadSchematicaPalette(const priv::NamedBinaryTag &schematic, SchematicPalette &mcpal) const;

	int parsePalette(const priv::NamedBinaryTag &schematic, SchematicPalette &mcpal) const;
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
public:
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Minecraft schematic",
									   "",
									   {"schematic", "schem", "nbt", "litematic", "bp"},
									   {},
									   VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE |
										   VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
