/**
 * @file
 */

#pragma once

#include "MeshFormat.h"

namespace voxelformat {

/**
 * @brief Crocotile 3D native scene format (.crocotile)
 *
 * JSON-based tile scene from https://crocotile3d.com/
 *
 * Structure:
 * - config: tilesizeX/Y, skybox, camera, baseUnit, ...
 * - model[]: tilesets with embedded texture (data URL) and optional scene tiles in object[]
 * - prefabs[]: reusable objects (tiles in object[]) placed via instances[] with TRS hierarchy
 *
 * Each tile is a textured quad (4 vertices, 2 triangles) with UVs and optional vertex colors.
 * Tile vertices are local to tile.position; prefab instances apply nested transforms.
 * Thin tiles are extruded along their normal (based on tilesize) so voxformat_scale does not
 * open gaps between stacked layers during voxelization.
 *
 * @ingroup Formats
 */
class CrocotileFormat : public MeshFormat {
protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;
	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &,
					const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
		return false;
	}

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Crocotile 3D", "", {"crocotile"}, {}, VOX_FORMAT_FLAG_MESH};
		return f;
	}
};

} // namespace voxelformat
