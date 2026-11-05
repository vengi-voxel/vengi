/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "json/JSON.h"
#include "math/Axis.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Pixelorama project format (*.pxo)
 *
 * A .pxo file is a zip archive with data.json and raw RGBA layer images under
 * image_data/frames/<frame>/layer_<cel>. Optional preview.png and blended
 * final_images are supported.
 *
 * Godot 3D layers under scene/<index> are voxelized via GodotSceneFormat.
 * Voxelorama cel metadata VoxelDepth extrudes pixels along Z (depth tool).
 *
 * https://github.com/Orama-Interactive/Pixelorama
 * https://github.com/Orama-Interactive/VoxeloramaExtension
 * https://pixelorama.org/Introduction
 *
 * @ingroup Formats
 */
class PxoFormat : public RGBASinglePaletteFormat {
protected:
	bool addFrame(scenegraph::SceneGraph &sceneGraph, const core::String &name, const palette::Palette &palette,
				  const image::ImagePtr &image, int frameIndex, math::Axis axis, int offset) const;
	bool addVoxeloramaFrame(scenegraph::SceneGraph &sceneGraph, const core::String &name,
							const palette::Palette &palette, const io::ArchivePtr &zip, const json::Json &root,
							int canvasWidth, int canvasHeight, int frameIndex) const;
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Pixelorama",
									   "application/x-pixelorama",
									   {"pxo"},
									   {},
									   VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED |
										   FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
