/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * @brief VoxEdit (Sandbox) (vxr)
 */
class VXRFormat : public Format {
private:
	bool loadChildVXM(const core::String& vxmPath, SceneGraph& sceneGraph);
	bool importChild(const core::String& vxmPath, io::SeekableReadStream& stream, SceneGraph& sceneGraph, uint32_t version);
	bool importChildOld(const core::String &filename, io::SeekableReadStream& stream, uint32_t version);
	bool saveRecursiveNode(const core::String &name, const voxel::SceneGraphNode& node, const core::String &filename, io::SeekableWriteStream& stream);
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
