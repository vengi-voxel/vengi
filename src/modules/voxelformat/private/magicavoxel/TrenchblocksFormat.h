/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

struct ogt_vox_scene;

namespace voxelformat {

/**
 * @brief Trenchblocks map import and export (four fixed MagicaVoxel tiles).
 * @ingroup Formats
 */
class TrenchblocksFormat : public Format {
protected:
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	static bool isTrenchblocksScene(const ogt_vox_scene *scene);
	static bool loadScene(const ogt_vox_scene *scene, scenegraph::SceneGraph &sceneGraph);

	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"Trenchblocks", "", {"vox"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
