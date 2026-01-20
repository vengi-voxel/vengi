/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Minecraft level dat format
 *
 * https://minecraft.wiki/w/Level.dat
 *
 * @ingroup Formats
 */
class DatFormat : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override {
		return false;
	}
public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Minecraft level dat", "", {"dat"}, {}, 0u};
		return f;
	}
};

} // namespace voxelformat
