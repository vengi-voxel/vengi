/**
 * @file
 */

#pragma once

#include "MeshFormat.h"

namespace io {
class ZipArchive;
}

namespace voxelformat {

/**
 * @brief 3D Manufacturing Format (3MF)
 *
 * OPC zip package with a core mesh for slicers (millimetres, Z up, one voxel = 1 mm).
 * Triangle colors use the materials colorgroup. With @c voxformat_3mfvolumetric the same
 * mesh also carries a volumetric image3d (PNG slice stack) so interior voxels survive.
 *
 * @ingroup Formats
 */
class ThreeMFFormat : public MeshFormat {
private:
	bool writePackage(io::ZipArchive &zip, const scenegraph::SceneGraph &sceneGraph, const ChunkMeshes &meshes,
					  const math::Vec3f &scale, bool withColor, bool volumetric, bool (*stop)());
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;

public:
	bool collectingBounds() const {
		return collectingInputBounds();
	}
	int importLoadedMesh(const core::String &name, scenegraph::SceneGraph &sceneGraph, Mesh &&mesh) const {
		return importMesh(name, sceneGraph, core::move(mesh));
	}

	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &meshes,
					const core::String &filename, const io::ArchivePtr &archive, const math::Vec3f &scale, bool quad,
					bool withColor, bool withTexCoords) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"3D Manufacturing Format",
									   "model/3mf",
									   {"3mf"},
									   {},
									   VOX_FORMAT_FLAG_MESH | VOX_FORMAT_FLAG_RGB | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
