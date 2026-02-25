/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"

namespace voxelformat {

/**
 * This format is our own format which stores a scene graph node hierarchy.
 *
 * It's a RIFF header based format. It stores one palette per model node.
 *
 * @ingroup Formats
 */
class VENGIFormat : public Format {
private:
	using NodeMapping = core::DynamicMap<int, int>;

	bool saveNodeProperties(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
							io::WriteStream &stream);
	bool saveNodeData(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
					  io::WriteStream &stream);
	bool saveAnimation(const scenegraph::SceneGraphNode &node, const core::String &animation, io::WriteStream &stream);
	bool saveNodeKeyFrame(const scenegraph::SceneGraphKeyFrame &keyframe, io::WriteStream &stream);
	bool saveNodePaletteColors(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
							   io::WriteStream &stream);
	bool saveNodePaletteIdentifier(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
								   io::WriteStream &stream);
	bool saveNodePaletteNormals(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
								io::WriteStream &stream);
	bool saveIKConstraint(const scenegraph::SceneGraphNode &node, io::WriteStream &stream);
	bool saveNode(const scenegraph::SceneGraph &sceneGraph, io::WriteStream &stream,
				  const scenegraph::SceneGraphNode &node);

	bool loadNodeProperties(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version,
							io::ReadStream &stream);
	bool loadNodeData(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version,
					  io::ReadStream &stream);
	bool loadAnimation(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version,
					   io::ReadStream &stream);
	bool loadNodeKeyFrame(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version,
						  io::ReadStream &stream);
	bool loadNodePaletteColors(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version,
							   io::ReadStream &stream);
	bool loadNodePaletteIdentifier(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
								   uint32_t version, io::ReadStream &stream);
	bool loadNodePaletteNormals(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version,
								io::ReadStream &stream);
	bool loadIKConstraint(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version,
						  io::ReadStream &stream);
	bool loadNode(scenegraph::SceneGraph &sceneGraph, int parent, uint32_t version, io::ReadStream &stream,
				  NodeMapping &nodeMapping);

public:
	bool supportsReferences() const override {
		return true;
	}
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"Vengi", "", {"vengi"}, {"VENG"}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_ANIMATION | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
