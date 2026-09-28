/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief VoxelCdx project format (*.vcdx)
 *
 * Binary header `VCDX` + version, optional preview JSON/PNG (v4+), then a Brotli-compressed
 * body with a 1024-slot palette, models, 32^3 u16 chunks, and an optional rig.
 *
 * VoxelCdx models are switchable documents; only layers of the active model composite.
 * Export writes one model whose layers are the vengi model nodes (world TRS baked).
 * Import maps a multi-layer model to a group of child nodes.
 *
 * Writers emit version 10. Readers accept versions 1-10.
 *
 * Coordinate system is Y-up, matching vengi. Palette index 0 is empty.
 *
 * https://github.com/sazixworkbench/voxelcdx-releases/blob/main/docs/vcdx-format.md
 *
 * @ingroup Formats
 */
class VCdxFormat : public PaletteFormat {
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
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"VoxelCdx",
									   "",
									   {"vcdx"},
									   {"VCDX"},
									   VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED | VOX_FORMAT_FLAG_PALETTE_EMBEDDED |
										   FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
