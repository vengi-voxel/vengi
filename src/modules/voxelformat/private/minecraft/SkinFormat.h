/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Load the minecraft skin data png into separate volumes
 *
 * https://assets.mojang.com/SkinTemplates/steve.png
 * https://assets.mojang.com/SkinTemplates/alex.png
 * https://minecraft.wiki/w/Skin
 */
class SkinFormat : public RGBAFormat {
public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							   const LoadContext &ctx) override;
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;

	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Minecraft skin", "", {"mcskin", "png"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
