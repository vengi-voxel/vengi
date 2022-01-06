/**
 * @file
 */

#include "MeshExporter.h"
#include "core/Log.h"
#include "core/Var.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"

namespace voxel {

MeshExporter::MeshExt::MeshExt(voxel::Mesh *_mesh, const core::String &_name) :
		mesh(_mesh), name(_name) {
}

bool MeshExporter::loadGroups(const core::String &filename, io::SeekableReadStream& file, SceneGraph& volumes) {
	Log::debug("Meshes can't get voxelized yet");
	return false;
}

bool MeshExporter::saveGroups(const SceneGraph& volumes, const core::String &filename, io::SeekableWriteStream& stream) {
	const bool mergeQuads = core::Var::get(cfg::VoxformatMergequads, "true", core::CV_NOPERSIST)->boolVal();
	const bool reuseVertices = core::Var::get(cfg::VoxformatReusevertices, "true", core::CV_NOPERSIST)->boolVal();
	const bool ambientOcclusion = core::Var::get(cfg::VoxformatAmbientocclusion, "false", core::CV_NOPERSIST)->boolVal();
	const float scale = core::Var::get(cfg::VoxformatScale, "1.0", core::CV_NOPERSIST)->floatVal();
	const bool quads = core::Var::get(cfg::VoxformatQuads, "true", core::CV_NOPERSIST)->boolVal();
	const bool withColor = core::Var::get(cfg::VoxformatWithcolor, "true", core::CV_NOPERSIST)->boolVal();
	const bool withTexCoords = core::Var::get(cfg::VoxformatWithtexcoords, "true", core::CV_NOPERSIST)->boolVal();

	Meshes meshes;
	for (const SceneGraphNode& v : volumes) {
		if (v.volume() == nullptr) {
			continue;
		}
		voxel::Mesh *mesh = new voxel::Mesh();
		voxel::Region region = v.region();
		region.shiftUpperCorner(1, 1, 1);
		voxel::extractCubicMesh(v.volume(), region, mesh, voxel::IsQuadNeeded(), glm::ivec3(0), mergeQuads, reuseVertices, ambientOcclusion);
		meshes.emplace_back(mesh, v.name());
	}
	Log::debug("Save meshes");
	const bool state = saveMeshes(meshes, filename, stream, scale, quads, withColor, withTexCoords);
	for (MeshExt& meshext : meshes) {
		delete meshext.mesh;
	}
	return state;
}

}
