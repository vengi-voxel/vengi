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
	bool saveNodeProperties(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, io::WriteStream &stream);
	bool saveNodeData(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, io::WriteStream &stream);
	bool saveAnimation(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, const core::String &animation, io::WriteStream &stream);
	bool saveNodeKeyFrame(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphKeyFrame &keyframe, io::WriteStream &stream);
	bool saveNodePaletteColors(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, io::WriteStream &stream);
	bool saveNodePaletteIdentifier(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node, io::WriteStream &stream);
	bool saveNode(const scenegraph::SceneGraph &sceneGraph, io::WriteStream &stream, const scenegraph::SceneGraphNode &node);

	bool loadNodeProperties(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadNodeData(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadAnimation(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadNodeKeyFrame(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadNodePaletteColors(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadNodePaletteIdentifier(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version, io::ReadStream &stream);
	bool loadNode(scenegraph::SceneGraph &sceneGraph, int parent, uint32_t version, io::ReadStream &stream);

protected:
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream &stream,
					const SaveContext &ctx) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;
};

} // namespace voxelformat
