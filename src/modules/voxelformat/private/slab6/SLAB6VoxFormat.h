/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief SLAB6 vox format
 *
 * @ingroup Formats
 */
class SLAB6VoxFormat : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
	int emptyPaletteIndex() const override {
		return 255;
	}

public:
	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"SLAB6 vox", "", {"vox"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
