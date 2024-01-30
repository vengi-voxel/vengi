/**
 * @file
 */

#include "MeshState.h"
#include "voxel/MaterialColor.h"

namespace voxelrender {

void MeshState::clear() {
	for (int i = 0; i < MeshType_Max; ++i) {
		for (auto &iter : _meshes[i]) {
			for (voxel::Mesh *mesh : iter.second) {
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

const palette::Palette &MeshState::palette(int idx) const {
	if (idx < 0 || idx > MAX_VOLUMES || !_volumeData[idx]._palette.hasValue()) {
		return voxel::getPalette();
	}
	return *_volumeData[idx]._palette.value();
}

void MeshState::setVolume(int idx, voxel::RawVolume *volume) {
	if (idx < 0 || idx > MAX_VOLUMES) {
		return;
	}
	_volumeData[idx]._rawVolume = volume;
}

void MeshState::setPalette(int idx, palette::Palette *palette) {
	if (idx < 0 || idx > MAX_VOLUMES) {
		return;
	}
	_volumeData[idx]._palette.setValue(palette);
}

core::DynamicArray<voxel::RawVolume *> MeshState::shutdown() {
	core::DynamicArray<voxel::RawVolume *> old(MAX_VOLUMES);
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		VolumeData &state = _volumeData[idx];
		// hand over the ownership to the caller
		old.push_back(state._rawVolume);
		state._rawVolume = nullptr;
	}
	return old;
}

void MeshState::resetReferences() {
	for (auto &s : _volumeData) {
		s._reference = -1;
	}
}

int MeshState::reference(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return -1;
	}
	return _volumeData[idx]._reference;
}

void MeshState::setReference(int idx, int referencedIdx) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return;
	}
	VolumeData &state = _volumeData[idx];
	state._reference = referencedIdx;
}

bool MeshState::hidden(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return true;
	}
	return _volumeData[idx]._hidden;
}

void MeshState::hide(int idx, bool hide) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return;
	}
	_volumeData[idx]._hidden = hide;
}

bool MeshState::grayed(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return true;
	}
	return _volumeData[idx]._gray;
}

void MeshState::gray(int idx, bool gray) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return;
	}
	_volumeData[idx]._gray = gray;
}

} // namespace voxelrender
