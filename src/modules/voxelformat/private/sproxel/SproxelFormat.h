/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Sproxel importer (csv)
 *
 * @li https://github.com/emilk/sproxel/blob/master/ImportExport.cpp
 *
 * @ingroup Formats
 */
class SproxelFormat : public RGBASinglePaletteFormat {
protected:
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Sproxel csv", "text/csv", {"csv"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE | VOX_FORMAT_FLAG_RGB};
		return f;
	}
};

} // namespace voxelformat
