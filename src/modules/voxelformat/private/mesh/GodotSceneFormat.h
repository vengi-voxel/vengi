/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"
#include "voxelformat/private/mesh/MeshFormat.h"

namespace voxelformat {

/**
 * @brief Godot 4.x scene exporter
 *
 * @li https://docs.godotengine.org/en/stable/contributing/development/file_formats/tscn.html
 *
 * @ingroup Formats
 */
class GodotSceneFormat : public MeshFormat {
protected:
	enum class WriterStage : uint8_t { SUB_RESOURCE, NODES };

	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override {
		return false; // loading is not yet supported
	}

	bool saveNode(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
				  const scenegraph::SceneGraphNode &node, io::SeekableWriteStream &stream, const ChunkMeshes &meshes,
				  int &subResourceId, WriterStage stage) const;

	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const glm::vec3 &scale = glm::vec3(1.0f), bool quad = false, bool withColor = true,
					bool withTexCoords = true) override;

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Godot Scene",
									   "",
									   {"escn"},
									   {"[gd_"},
									   VOX_FORMAT_FLAG_MESH | /* TODO: VOX_FORMAT_FLAG_ANIMATION | */ FORMAT_FLAG_SAVE | FORMAT_FLAG_NO_LOAD};
		return f;
	}
};

} // namespace voxelformat
