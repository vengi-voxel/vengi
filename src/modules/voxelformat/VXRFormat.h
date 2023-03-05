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
	bool onlyOnePalette() override { return false; }
	bool loadChildVXM(const core::String& vxmPath, SceneGraph &sceneGraph, SceneGraphNode &node, int version, const LoadContext &ctx);

	bool handleVersion8AndLater(io::SeekableReadStream& stream, SceneGraphNode &node, const LoadContext &ctx);
	bool importChild(const core::String& vxmPath, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent, const LoadContext &ctx);

	bool loadGroupsVersion4AndLater(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, const LoadContext &ctx);

	bool loadGroupsVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, const LoadContext &ctx);
	bool importChildVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent, const LoadContext &ctx);
	bool saveRecursiveNode(const SceneGraph& sceneGraph, const SceneGraphNode& node, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx);
	bool saveNodeProperties(const SceneGraphNode* node, io::SeekableWriteStream& stream);
	bool saveVXA(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const core::String &animation, const SaveContext &ctx);
	bool loadVXA(SceneGraph& sceneGraph, const core::String& vxaPath, const LoadContext &ctx);
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette, const LoadContext &ctx) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) override;
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream, const LoadContext &ctx) override;
};

}
