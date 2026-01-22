/**
 * @file
 */

#pragma once

#include "voxel/Face.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief VXB files are block tiles
 *
 * @ingroup Formats
 */
class VXBFormat : public PaletteFormat {
protected:
	void faceTexture(voxel::RawVolume &volume, const palette::Palette &palette, voxel::FaceNames face,
					 const image::ImagePtr &diffuse, const image::ImagePtr &emissive) const;
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Sandbox VoxEdit Block",
									"",
									{"vxb"},
									{"VXB1"},
									VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
