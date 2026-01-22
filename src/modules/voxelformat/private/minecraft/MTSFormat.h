/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * The Minetest/Luanti Schematic File Format (minetest)
 *
 * @note https://docs.luanti.org/for-creators/luanti-schematic-file-format/
 *
 * @ingroup Formats
 */
class MTSFormat : public PaletteFormat {
protected:
	bool singleVolume() const override {
		return true; // MTS files only support a single volume
	}
public:
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Luanti (Minetest)", "", {"mts"}, {}, FORMAT_FLAG_SAVE | VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
