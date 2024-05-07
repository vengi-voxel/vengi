/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief EveryGraph Voxel3D format (v3b is compressed - see http://advsys.net/ken/util/v3b2vox.zip)
 *
 * @ingroup Formats
 */
class V3AFormat : public RGBAFormat {
protected:
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	bool singleVolume() const override {
		return true;
	}
};

} // namespace voxelformat
