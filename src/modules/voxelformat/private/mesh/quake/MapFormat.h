/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/mesh/MeshFormat.h"
#include "math/Vec3f.h"

namespace voxelformat {
/**
 * @brief Quake map format
 *
 * https://quakewiki.org/wiki/Quake_Map_Format
 *
 * @ingroup Formats
 */
class MapFormat : public MeshFormat {
protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;
	bool parseBrush(const core::String &filename, const io::ArchivePtr &archive, io::SeekableReadStream &stream,
					MeshMaterialMap &materials, Mesh &mesh) const;
	bool parseEntity(const core::String &filename, const io::ArchivePtr &archive, io::SeekableReadStream &stream,
					 MeshMaterialMap &materials, Mesh &mesh, scenegraph::SceneGraphNodeProperties &props) const;

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const math::Vec3f &scale = math::Vec3f(1.0f), bool quad = false, bool withColor = true,
					bool withTexCoords = true) override {
		return false;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Quake Map", "", {"map"}, {}, VOX_FORMAT_FLAG_MESH};
		return f;
	}
};

} // namespace voxelformat
