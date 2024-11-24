/**
 * @file
 */

#pragma once

#include "palette/Palette.h"
#include "voxelformat/Format.h"

namespace io {
class ZipReadStream;
}

namespace voxelformat {

/**
 * @brief BenVoxel (ben.json) format.
 *
 * https://github.com/BenMcLean/Voxel2Pixel/blob/master/BenVoxel/README.md
 *
 * @ingroup Formats
 */
class BenVoxelFormat : public PaletteFormat {
protected:
	int emptyPaletteIndex() const override;
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"BenVoxel", {"ben.json", "ben"}, {/*"BENV"*/}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
