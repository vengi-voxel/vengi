/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "voxelformat/Format.h"

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
	struct NodeSpec {
		core::String name;
		core::String modelName;
		core::String thingLibraryId;
		float opacity = 1.0f;
		glm::vec3 localPos{0.0f};
		glm::vec3 localRot{0.0f};
		glm::vec3 localSize{0.0f};
		core::RGBA color{0, 0, 0, 255};
		core::DynamicArray<NodeSpec> children;
		// TODO renderSpec and animSpec might be interesting, too
	};
	bool parseNode(core::Tokenizer &tok, NodeSpec &nodeSpec) const;
	bool parseChildren(core::Tokenizer &tok, NodeSpec &nodeSpec) const;

	bool loadNodeSpec(io::SeekableReadStream &stream, NodeSpec &nodeSpec) const;
	bool loadNode(const io::ArchivePtr &archive, const NodeSpec &nodeSpec, scenegraph::SceneGraph &sceneGraph,
				  const LoadContext &ctx, int parent = 0);

public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;
};

} // namespace voxelformat
