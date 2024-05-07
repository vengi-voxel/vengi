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
};

} // namespace voxelformat
