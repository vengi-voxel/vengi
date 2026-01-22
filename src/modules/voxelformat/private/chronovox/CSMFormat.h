/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Chronovox Studio Model and Nick's Voxel Model
 *
 * @ingroup Formats
 */
class CSMFormat : public RGBAFormat {
protected:
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Chronovox", "", {"csm"}, {".CSM"}, 0u};
		return f;
	}

	static const io::FormatDescription &formatNVM() {
		static io::FormatDescription f{"Nicks Voxel Model", "", {"nvm"}, {".NVM"}, 0u};
		return f;
	}
};

} // namespace voxelformat
