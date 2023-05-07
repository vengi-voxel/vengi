/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief VoxEdit (Sandbox) (vxr)
 * Transforms - since version 4 or higher the animations are part of a vxa file
 * They are designed to run at 24 fps
 * @sa VXMFormat
 * @sa VXAFormat
 *
 * @ingroup Formats
 */
class VXRFormat : public PaletteFormat {
private:
	bool onlyOnePalette() override {
		return false;
	}
	bool loadChildVXM(const core::String &vxmPath, scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
					  int version, const LoadContext &ctx);

	bool handleVersion8AndLater(io::SeekableReadStream &stream, scenegraph::SceneGraphNode &node,
								const LoadContext &ctx);
	bool importChild(const core::String &vxmPath, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
					 int version, int parent, const LoadContext &ctx);

	bool loadGroupsVersion4AndLater(const core::String &filename, io::SeekableReadStream &stream,
									scenegraph::SceneGraph &sceneGraph, int version, const LoadContext &ctx);

	bool loadGroupsVersion3AndEarlier(const core::String &filename, io::SeekableReadStream &stream,
									  scenegraph::SceneGraph &sceneGraph, int version, const LoadContext &ctx);
	bool importChildVersion3AndEarlier(const core::String &filename, io::SeekableReadStream &stream,
									   scenegraph::SceneGraph &sceneGraph, int version, int parent,
									   const LoadContext &ctx);
	bool saveRecursiveNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
						   const core::String &filename, io::SeekableWriteStream &stream, const SaveContext &ctx);
	bool saveNodeProperties(const scenegraph::SceneGraphNode *node, io::SeekableWriteStream &stream);
	bool saveVXA(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
				 io::SeekableWriteStream &stream, const core::String &animation, const SaveContext &ctx);
	bool loadVXA(scenegraph::SceneGraph &sceneGraph, const core::String &vxaPath, const LoadContext &ctx);
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;

public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream &stream,
								   const LoadContext &ctx) override;
};

} // namespace voxelformat
