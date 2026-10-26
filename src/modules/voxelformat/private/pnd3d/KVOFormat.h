/**
 * @file
 */
#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Ken's Voxel Octree, as used by PND3D.
 *
 * Loads kvo files for the PND3D engine by Ken Silverman (http://advsys.net/ken)
 *
 * @ingroup Formats
 */
class KVOFormat : public Format {
protected:
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &, const core::String &, const io::ArchivePtr &,
					const SaveContext &) override;

public:
	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Ken's Voxel Octree",
									   "",
									   {"kvo", "kvs", "kvn"},
									   {"KVOx"},
									   VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_RGB | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
