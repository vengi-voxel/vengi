/**
 * @file
 */

#include "MeshState.h"
#include "core/Log.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/SurfaceExtractor.h"

namespace voxelrender {

bool MeshState::init() {
	_meshMode = core::Var::getSafe(cfg::VoxelMeshMode);
	_meshMode->markClean();

	_threadPool.init();
	Log::debug("Threadpool size: %i", (int)_threadPool.size());
	return true;
}

void MeshState::construct() {
	_meshSize = core::Var::get(cfg::VoxelMeshSize, "64", core::CV_READONLY);
}

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

int MeshState::pop() {
	MeshState::ExtractionCtx result;
	while (_pendingQueue.pop(result)) {
		if (_volumeData[result.idx]._rawVolume == nullptr) {
			continue;
		}
		MeshState::Meshes &meshes = _meshes[MeshType_Opaque][result.mins];
		delete meshes[result.idx];
		meshes[result.idx] = new voxel::Mesh(core::move(result.mesh.mesh[MeshType_Opaque]));
		MeshState::Meshes &meshesT = _meshes[MeshType_Transparency][result.mins];
		delete meshesT[result.idx];
		meshesT[result.idx] = new voxel::Mesh(core::move(result.mesh.mesh[MeshType_Transparency]));
		return result.idx;
	}
	return -1;
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

voxel::Region MeshState::calculateExtractRegion(int x, int y, int z, const glm::ivec3 &meshSize) const {
	const glm::ivec3 mins(x * meshSize.x, y * meshSize.y, z * meshSize.z);
	const glm::ivec3 maxs = mins + meshSize - 1;
	return voxel::Region{mins, maxs};
}

bool MeshState::scheduleExtractions(size_t maxExtraction) {
	const size_t n = _extractRegions.size();
	if (n == 0) {
		return false;
	}
	if (maxExtraction == 0) {
		return true;
	}
	const bool marchingCubes = _meshMode->intVal() == 1;
	size_t i;
	for (i = 0; i < n; ++i) {
		ExtractRegion extractRegion;
		if (!_extractRegions.pop(extractRegion)) {
			break;
		}
		const int idx = extractRegion.idx;
		const voxel::RawVolume *v = volume(idx);
		if (v == nullptr) {
			continue;
		}
		const voxel::Region &finalRegion = extractRegion.region;
		bool onlyAir = true;
		const voxel::Region copyRegion(finalRegion.getLowerCorner() - 2, finalRegion.getUpperCorner() + 2);
		if (!copyRegion.isValid()) {
			continue;
		}
		voxel::RawVolume copy(v, copyRegion, &onlyAir);
		const glm::ivec3 &mins = finalRegion.getLowerCorner();
		if (!onlyAir) {
			const palette::Palette &pal = palette(resolveIdx(idx));
			++_pendingExtractorTasks;
			_threadPool.enqueue([marchingCubes, movedPal = core::move(pal), movedCopy = core::move(copy), mins, idx,
								 finalRegion, this]() {
				++_runningExtractorTasks;
				voxel::ChunkMesh mesh(65536, 65536, true);
				voxel::SurfaceExtractionContext ctx =
					marchingCubes ? voxel::buildMarchingCubesContext(&movedCopy, finalRegion, mesh, movedPal)
								  : voxel::buildCubicContext(&movedCopy, finalRegion, mesh, mins);
				voxel::extractSurface(ctx);
				_pendingQueue.emplace(mins, idx, core::move(mesh));
				Log::debug("Enqueue mesh for idx: %i (%i:%i:%i)", idx, mins.x, mins.y, mins.z);
				--_runningExtractorTasks;
				--_pendingExtractorTasks;
			});
		} else {
			_pendingQueue.emplace(mins, idx, core::move(voxel::ChunkMesh(0, 0)));
		}
		--maxExtraction;
		if (maxExtraction == 0) {
			break;
		}
	}

	return true;
}

bool MeshState::update() {
	bool triggerClear = false;
	if (_meshMode->isDirty()) {
		_meshMode->markClean();
		clearPendingExtractions();

		for (int i = 0; i < MAX_VOLUMES; ++i) {
			if (voxel::RawVolume *v = volume(i)) {
				extractRegion(i, v->region());
			}
		}
		triggerClear = true;
	}
	scheduleExtractions();
	return triggerClear;
}

bool MeshState::extractRegion(int idx, const voxel::Region &region) {
	core_trace_scoped(RawVolumeRendererExtract);
	const int bufferIndex = resolveIdx(idx);
	voxel::RawVolume *v = volume(bufferIndex);
	if (v == nullptr) {
		return false;
	}

	const int s = _meshSize->intVal();
	const glm::ivec3 meshSize(s);
	const glm::ivec3 meshSizeMinusOne(s - 1);
	const voxel::Region &completeRegion = v->region();

	// convert to step coordinates that are needed to extract
	// the given region mesh size ranges
	// the boundaries are special - that's why we take care of this with
	// the offset of 1 - see the cubic surface extractor docs
	const glm::ivec3 &l = (region.getLowerCorner() - meshSizeMinusOne) / meshSize;
	const glm::ivec3 &u = (region.getUpperCorner() + 1) / meshSize;

	bool deletedMesh = false;
	Log::debug("modified region: %s", region.toString().c_str());
	for (int x = l.x; x <= u.x; ++x) {
		for (int y = l.y; y <= u.y; ++y) {
			for (int z = l.z; z <= u.z; ++z) {
				const voxel::Region &finalRegion = calculateExtractRegion(x, y, z, meshSize);
				const glm::ivec3 &mins = finalRegion.getLowerCorner();

				if (!voxel::intersects(completeRegion, finalRegion)) {
					deleteMeshes(mins, bufferIndex);
					deletedMesh = true;
					continue;
				}

				Log::debug("extract region: %s", finalRegion.toString().c_str());
				_extractRegions.emplace(finalRegion, bufferIndex, hidden(bufferIndex));
			}
		}
	}
	return deletedMesh;
}

void MeshState::extractAll() {
	while (scheduleExtractions(100)) {
	}
	waitForPendingExtractions();
}

void MeshState::waitForPendingExtractions() {
	while (_pendingExtractorTasks > 0) {
		SDL_Delay(1);
	}
}

void MeshState::clearPendingExtractions() {
	Log::debug("Clear pending extractions");
	_threadPool.abort();
	while (_runningExtractorTasks > 0) {
		SDL_Delay(1);
	}
	_pendingQueue.clear();
	_pendingExtractorTasks = 0;
}

bool MeshState::marchingCubes() const {
	return _meshMode->intVal() == 1;
}

int MeshState::resolveIdx(int idx) const {
	const int ref = reference(idx);
	if (ref != -1) {
		return resolveIdx(ref);
	}
	return idx;
}

voxel::RawVolume *MeshState::setVolume(int idx, voxel::RawVolume *v, palette::Palette *palette, bool meshDelete,
									   bool &meshDeleted) {
	meshDeleted = false;
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	_volumeData[idx]._palette.setValue(palette);
	voxel::RawVolume *old = volume(idx);
	if (old == v) {
		return nullptr;
	}
	core_trace_scoped(RawVolumeRendererSetVolume);
	_volumeData[idx]._rawVolume = v;
	if (meshDelete) {
		deleteMeshes(idx);
		meshDeleted = true;
	}
	const size_t n = _extractRegions.size();
	for (size_t i = 0; i < n; ++i) {
		if (_extractRegions[i].idx == idx) {
			_extractRegions[i].idx = -1;
		}
	}

	return old;
}

core::DynamicArray<voxel::RawVolume *> MeshState::shutdown() {
	_threadPool.shutdown();
	clear();
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
