/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief BrickLink Studio model format (.io)
 *
 * An .io file is a zip archive that contains an LDraw model as model.ldr (or model2.ldr).
 * Newer archives are encrypted with the password "soho0909".
 *
 * Format Structure
 *
 * The zip archive typically contains:
 * - model.ldr - the primary LDraw model file
 * - model2.ldr - alternative model with embedded part definitions (fallback)
 * - thumbnail.png - preview image
 *
 * Import Behavior
 *
 * This format simply delegates to LDrawFormat after extracting the .ldr file from the zip.
 * It first tries model.ldr (standard LDraw with external part references), and if that fails,
 * falls back to model2.ldr (which may contain inline part geometry).
 *
 * All coordinate conversion and transform handling is identical to LDrawFormat since the
 * contained file IS an LDraw file.
 *
 * @see https://wiki.ldraw.org/wiki/IO
 *
 * @ingroup Formats
 */
class StudioIOFormat : public Format {
protected:
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override {
		return false;
	}

public:
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"BrickLink Studio", "", {"io"}, {}, VOX_FORMAT_FLAG_MESH | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
