/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief VXT files are tilesets of (static) vxm models
 *
 * @ingroup Formats
 */
class VXTFormat : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Sandbox VoxEdit Tilemap", "", {"vxt"}, {"VXT1"}, 0u};
		return f;
	}
};

} // namespace voxelformat
