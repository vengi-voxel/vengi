/**
 * @file
 */

#pragma once

#include "Format.h"
#include "voxel/RawVolume.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelformat {

/**
 * This format is our own format which stores a scene graph node hierarchy
 * with animation and (lua-)script support.
 *
 * It's a RIFF header based format. It stores one palette per model node.
 */
class VENGIFormat : public Format {
private:
	bool saveNodeProperties(const SceneGraph &sceneGraph, const SceneGraphNode &node, io::WriteStream &stream);
	bool saveNodeData(const SceneGraph &sceneGraph, const SceneGraphNode &node, io::WriteStream &stream);
	bool saveAnimation(const SceneGraph &sceneGraph, const SceneGraphNode &node, const core::String &animation, io::WriteStream &stream);
	bool saveNodeKeyFrame(const SceneGraph &sceneGraph, const SceneGraphKeyFrame &keyframe, io::WriteStream &stream);
	bool saveNodePaletteColors(const SceneGraph &sceneGraph, const SceneGraphNode &node, io::WriteStream &stream);
	bool saveNodePaletteIdentifier(const SceneGraph &sceneGraph, const SceneGraphNode &node, io::WriteStream &stream);
	bool saveNode(const SceneGraph &sceneGraph, io::WriteStream &stream, const SceneGraphNode &node);

	bool loadNodeProperties(SceneGraph &sceneGraph, SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadNodeData(SceneGraph &sceneGraph, SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadAnimation(SceneGraph &sceneGraph, SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadNodeKeyFrame(SceneGraph &sceneGraph, SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadNodePaletteColors(SceneGraph &sceneGraph, SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadNodePaletteIdentifier(SceneGraph &sceneGraph, SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadNode(SceneGraph &sceneGraph, int parent, uint32_t version, io::ReadStream &stream);

protected:
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream &stream,
					const SaveContext &ctx) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, const LoadContext &ctx) override;
};

} // namespace voxelformat
