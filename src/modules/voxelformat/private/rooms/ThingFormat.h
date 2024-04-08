/**
 * @file
 */

#pragma once

#include "io/Archive.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief rooms.xyz (thing)
 *
 * This is a wrapper around the magicavoxel format that adds further information to the nodes with a text file.
 * @sa VoxFormat
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
		// TODO renderSpec and animSpec might be interesting, too
	};
	bool loadNodeSpec(io::SeekableReadStream &stream, NodeSpec &nodeSpec) const;

	bool loadNode(const io::ArchivePtr &archive, const NodeSpec &nodeSpec, scenegraph::SceneGraph &sceneGraph,
				  const LoadContext &ctx);

public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;
};

} // namespace voxelformat
