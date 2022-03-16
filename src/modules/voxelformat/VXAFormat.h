/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief VoxEdit (Sandbox) (vxa)
 */
class VXAFormat : public Format {
	bool recursiveImportNode(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, SceneGraphNode& node, const core::String &animId);
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override {
		return false;
	}
};

}
