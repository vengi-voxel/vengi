/**
 * @file
 */

#pragma once

#include "CubzhFormat.h"

namespace voxelformat {

/**
 * @ingroup Formats
 */
class PCubesFormat : public CubzhFormat {
protected:
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Particubes",
									"",
									{"pcubes", "particubes"},
									{"PARTICUBES!"},
									VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED |
										FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
