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

bool MeshState::deleteMeshes(const glm::ivec3 &pos, int idx) {
	bool d = false;
	for (int i = 0; i < MeshType_Max; ++i) {
		auto &meshes = _meshes[i];
		auto iter = meshes.find(pos);
		if (iter != meshes.end()) {
			MeshState::Meshes &array = iter->second;
			voxel::Mesh *mesh = array[idx];
			delete mesh;
			array[idx] = nullptr;
			d = true;
		}
	}
	return d;
}

bool MeshState::deleteMeshes(int idx) {
	bool d = false;
	for (int i = 0; i < MeshType_Max; ++i) {
		auto &meshes = _meshes[i];
		for (auto &iter : meshes) {
			MeshState::Meshes &array = iter.second;
			voxel::Mesh *mesh = array[idx];
			delete mesh;
			array[idx] = nullptr;
			d = true;
		}
	}
	return d;
}

const MeshState::MeshesMap &MeshState::meshes(MeshType type) const {
	return _meshes[type];
}

bool MeshState::empty(int idx) const {
	for (int i = 0; i < MeshType_Max; ++i) {
		for (auto &m : _meshes[i]) {
			const MeshState::Meshes &meshes = m.second;
			if (meshes[idx] != nullptr && meshes[idx]->getNoOfIndices() > 0) {
				return false;
			}
		}
	}
	return true;
}

void MeshState::count(MeshType meshType, int idx, size_t &vertCount, size_t &normalsCount, size_t &indCount) const {
	for (auto &i : _meshes[meshType]) {
		const MeshState::Meshes &meshes = i.second;
		const voxel::Mesh *mesh = meshes[idx];
		if (mesh == nullptr || mesh->getNoOfIndices() <= 0) {
			continue;
		}
		const voxel::VertexArray &vertexVector = mesh->getVertexVector();
		const voxel::NormalArray &normalVector = mesh->getNormalVector();
		const voxel::IndexArray &indexVector = mesh->getIndexVector();
		vertCount += vertexVector.size();
		normalsCount += normalVector.size();
		indCount += indexVector.size();
	}
}

} // namespace voxelrender
