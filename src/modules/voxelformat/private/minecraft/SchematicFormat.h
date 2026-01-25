/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

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
