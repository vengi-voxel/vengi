/**
 * @file
 */

#include "MeshState.h"

namespace voxelrender {

void MeshState::clear() {
	for (int i = 0; i < MeshType_Max; ++i) {
		for (auto &iter : _meshes[i]) {
			for (auto &mesh : iter.second) {
				delete mesh;
			}
		}
		_meshes[i].clear();
	}
}

void MeshState::setOpaque(const glm::ivec3 &pos, int idx, voxel::Mesh &&mesh) {
	MeshState::Meshes &meshes = _meshes[MeshType_Opaque][pos];
	delete meshes[idx];
	meshes[idx] = new voxel::Mesh(mesh);
}

void MeshState::setTransparent(const glm::ivec3 &pos, int idx, voxel::Mesh &&mesh) {
	MeshState::Meshes &meshesT = _meshes[MeshType_Transparency][pos];
	delete meshesT[idx];
	meshesT[idx] = new voxel::Mesh(mesh);
}

void MeshState::set(ExtractionCtx &ctx) {
	setOpaque(ctx.mins, ctx.idx, core::move(ctx.mesh.mesh[MeshType_Opaque]));
	setTransparent(ctx.mins, ctx.idx, core::move(ctx.mesh.mesh[MeshType_Transparency]));
}

} // namespace voxelrender
