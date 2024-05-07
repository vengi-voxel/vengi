/**
 * @file
 */

#pragma once

#include "io/Archive.h"
#include "voxelformat/Format.h"

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
	bool recursiveImportNodeSince3(const core::String &filename, io::SeekableReadStream &stream,
								   scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
								   const core::String &animId, int version);
	bool recursiveImportNodeBefore3(const core::String &filename, io::SeekableReadStream &stream,
									scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
									const core::String &animId, int version);
	bool saveRecursiveNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
						   const core::String &animation, const core::String &filename,
						   io::SeekableWriteStream &stream);
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
};

} // namespace voxelformat
