/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief VoxEdit (Sandbox) (vxm)
 * The voxel model
 * @sa VXAFormat
 * @sa VXRFormat
 *
 * @ingroup Formats
 */
class VXMFormat : public PaletteFormat {
private:
	bool writeRLE(io::WriteStream &stream, int rleCount, const voxel::Voxel &voxel, const palette::Palette &nodePalette,
				  const palette::Palette &palette) const;
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Sandbox VoxEdit Model",
									"",
									{"vxm"},
									{"VXMA", "VXMB", "VXMC", "VXM9", "VXM8", "VXM7", "VXM6", "VXM5", "VXM4", "VXM3"},
									VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
