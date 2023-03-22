/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

#define SANDBOX_CONTROLLER_NODE "Controller"

/**
 * @brief VoxEdit (Sandbox) (vxa)
 * Animation file that together with vxr and vxm files form the full asset
 * @sa VXMFormat
 * @sa VXRFormat
 *
 * @ingroup Formats
 */
class VXAFormat : public Format {
private:
	bool recursiveImportNodeSince3(const core::String &filename, io::SeekableReadStream& stream, scenegraph::SceneGraph& sceneGraph, scenegraph::SceneGraphNode& node, const core::String &animId, int version);
	bool recursiveImportNodeBefore3(const core::String &filename, io::SeekableReadStream& stream, scenegraph::SceneGraph& sceneGraph, scenegraph::SceneGraphNode& node, const core::String &animId, int version);
	bool saveRecursiveNode(const scenegraph::SceneGraph& sceneGraph, const scenegraph::SceneGraphNode& node, const core::String &filename, io::SeekableWriteStream& stream);
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, scenegraph::SceneGraph& sceneGraph, const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, const SaveContext &ctx) override;
};

}
