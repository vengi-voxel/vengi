/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Load the minecraft skin data png into separate volumes
 */
class SkinFormat : public RGBAFormat {
public:
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const palette::Palette &palette, const LoadContext &ctx) override;

	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Minecraft skin", {"mcskin", "png"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
