/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief VoxEdit (Sandbox) (vxr)
 * Transforms - since version 4 or higher the animations are part of a vxa file
 * @sa VXMFormat
 * @sa VXAFormat
 *
 * @ingroup Formats
 */
class VXRFormat : public PaletteFormat {
private:
	bool loadChildVXM(const core::String& vxmPath, SceneGraph &sceneGraph, SceneGraphNode &node, int version);

	bool handleVersion8AndLater(io::SeekableReadStream& stream, SceneGraphNode &node);
	bool importChild(const core::String& vxmPath, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent);

	bool loadGroupsVersion4AndLater(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version);

	bool loadGroupsVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version);
	bool importChildVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent);
	bool saveRecursiveNode(const SceneGraph& sceneGraph, const SceneGraphNode& node, const core::String &filename, io::SeekableWriteStream& stream);
	bool saveNodeProperties(const SceneGraphNode* node, io::SeekableWriteStream& stream);
	bool saveVXA(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const core::String &animation);
	bool loadVXA(SceneGraph& sceneGraph, const core::String& vxaPath);
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
