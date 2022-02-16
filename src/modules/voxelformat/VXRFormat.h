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
	bool loadChildVXM(const core::String& vxmPath, voxel::SceneGraphNode &node, int version);

	bool handleVersion8AndLater(io::SeekableReadStream& stream, voxel::SceneGraphNode &node);
	bool importChild(const core::String& vxmPath, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent);

	bool loadGroupsVersion4AndLater(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version);

	bool loadGroupsVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version);
	bool importChildVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent);
	bool saveRecursiveNode(const core::String &name, const voxel::SceneGraphNode& node, const core::String &filename, io::SeekableWriteStream& stream);
	bool loadVXA(SceneGraph& sceneGraph, const core::String& vxaPath);
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
