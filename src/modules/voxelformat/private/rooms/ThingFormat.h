/**
 * @file
 */

#pragma once

#include "io/Archive.h"
#include "voxelformat/Format.h"
#include "ThingNodeParser.h"

namespace core {
class Tokenizer;
}

namespace voxelformat {

/**
 * @brief rooms.xyz (thing)
 *
 * This is a wrapper around the magicavoxel format that adds further information to the nodes with a text file.
 * @sa VoxFormat
 *
 * @todo some thing files contain a icon.png (128x128) thumbnail, which could be used for the thumbnailer
 *
 * @ingroup Formats
 */
class ThingFormat : public Format {
private:
	bool loadNodeSpec(io::SeekableReadStream &stream, NodeSpec &nodeSpec) const;
	void addMediaImage(const io::ArchivePtr &archive, const NodeSpec &nodeSpec, scenegraph::SceneGraph &voxSceneGraph);
	bool loadNode(const io::ArchivePtr &archive, const NodeSpec &nodeSpec, scenegraph::SceneGraph &sceneGraph,
				  const LoadContext &ctx, int parent = 0);

public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;
};

} // namespace voxelformat
