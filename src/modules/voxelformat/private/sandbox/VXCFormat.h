/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief VXC files are just a list of compressed files
 *
 * @ingroup Formats
 */
class VXCFormat : public Format {
protected:
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
public:
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Sandbox VoxEdit Collection", "", {"vxc"}, {}, VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
