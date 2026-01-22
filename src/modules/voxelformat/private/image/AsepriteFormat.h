/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

struct ase_t;

namespace voxelformat {

/**
 * @brief Aseprite format
 *
 * https://github.com/aseprite/aseprite/blob/main/docs/ase-file-specs.md
 * https://libresprite.github.io
 *
 * @ingroup Formats
 */
class AsepriteFormat : public RGBASinglePaletteFormat {
protected:
	bool addFrame(scenegraph::SceneGraph &sceneGraph, const core::String &filename, const palette::Palette &palette,
				  const LoadContext &ctx, const ase_t *ase, int frameIndex, math::Axis axis, int offset) const;
	ase_t *loadAseprite(const core::String &filename, const io::ArchivePtr &archive) const;
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override {
		return false;
	}

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"aseprite", "image/aseprite", {"aseprite", "ase"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
