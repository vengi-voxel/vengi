/**
 * @file
 */

#include "MeshState.h"
#include "app/App.h"
#include "app/ForParallel.h"
#include "app/I18NMarkers.h"
#include "core/Log.h"
#include "core/concurrent/Concurrency.h"
#include "palette/NormalPalette.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/SurfaceExtractor.h"

namespace voxel {

static constexpr int FullAllocVertices = 262144;
static constexpr int FullAllocIndices = 524288;
static constexpr int SmallAllocVertices = 1024;
static constexpr int SmallAllocIndices = 2048;

MeshState::MeshState() {
}

void MeshState::ensureSize(int idx) {
	if (idx < (int)_volumeData.size()) {
		return;
	}
	const int newSize = idx + 1;
	_volumeData.resize(newSize);
	_pendingMeshDirty.resize(newSize);
}

int MeshState::maxSize() const {
	return (int)_volumeData.size();
}

bool MeshState::init() {
	_meshMode = core::getVar(cfg::VoxelMeshMode);
	_meshMode->markClean();
	return true;
}

void MeshState::construct() {
	// this must be 62 for the binary cubic mesher
	const core::VarDef voxelMeshSize(cfg::VoxelMeshSize, 62, N_("Voxel Mesh Size"), N_("The size of the voxel mesh"),
									 core::CV_READONLY | core::CV_NOPERSIST);
	_meshSize = core::Var::registerVar(voxelMeshSize);
	// Editor/render mesh mode - excludes GreedyTexture as it's not supported by the renderer
	const core::VarDef voxRenderMeshMode(cfg::VoxelMeshMode, (int)voxel::SurfaceExtractionType::Binary,
										 (int)voxel::SurfaceExtractionType::Cubic,
										 (int)voxel::SurfaceExtractionType::Binary, N_("Mesh mode"),
										 N_("0 = cubes, 1 = marching cubes, 2 = binary mesher"), core::CV_SHADER);
	core::Var::registerVar(voxRenderMeshMode);
	const core::VarDef voxMeshAlloc(cfg::VoxelMeshAlloc, 0, 0, 1,
									N_("Mesh allocation strategy"),
									N_("0 = full pre-alloc (best for small models), 1 = small alloc (best for large models)"));
	_meshAlloc = core::Var::registerVar(voxMeshAlloc);
}

glm::vec3 MeshState::VolumeData::centerPos(bool applyModel) const {
	const glm::vec4 center((_mins + _maxs) * 0.5f, 1.0f);
	const glm::vec3 pos = applyModel ? _model * center : center;
	return pos;
}

const glm::vec3 &MeshState::mins(int idx) const {
	return _volumeData[idx]._mins;
}

const glm::vec3 &MeshState::maxs(int idx) const {
	return _volumeData[idx]._maxs;
}

glm::vec3 MeshState::centerPos(int idx, bool applyModel) const {
	return _volumeData[idx].centerPos(applyModel);
}

const glm::mat4 &MeshState::model(int idx) const {
	return _volumeData[idx]._model;
}

bool MeshState::setModelMatrix(int idx, const glm::mat4 &model, const glm::vec3 &mins, const glm::vec3 &maxs) {
	if (idx < 0) {
		Log::error("Given id %i is out of bounds", idx);
		return false;
	}
	ensureSize(idx);
	if (reference(idx) == -1 && volume(idx) == nullptr) {
		Log::error("No volume found at: %i", idx);
		return false;
	}
	VolumeData &state = _volumeData[idx];
	state._model = model;
	state._mins = mins;
	state._maxs = maxs;
	return true;
}

void MeshState::clearMeshes() {
	for (int i = 0; i < MeshType_Max; ++i) {
		for (const auto &iter : _meshes[i]) {
			for (voxel::Mesh *mesh : iter->value) {
				delete mesh;
			}
		}
		_meshes[i].clear();
	}
}

void MeshState::addOrReplaceMeshes(MeshState::ExtractionResult &result, MeshType type) {
	auto iter = _meshes[type].find(result.mins);
	auto &mesh = result.mesh.mesh[type];
	if (iter != _meshes[type].end()) {
		if (result.idx >= (int)iter->value.size()) {
			iter->value.resize(result.idx + 1);
		}
		delete iter->value[result.idx];
		if (mesh.isEmpty()) {
			iter->value[result.idx] = nullptr;
			return;
		}
		iter->value[result.idx] = new voxel::Mesh(core::move(mesh));
		return;
	}
	if (mesh.isEmpty()) {
		return;
	}
	Meshes meshes;
	meshes.resize(_volumeData.size());
	meshes[result.idx] = new voxel::Mesh(core::move(mesh));
	_meshes[type].emplace(result.mins, core::move(meshes));
}

int MeshState::pop() {
	int result = -1;
	_pendingMeshes.try_pop(result);
	if (result >= 0 && result < (int)_pendingMeshDirty.size()) {
		_pendingMeshDirty[result] = false;
	}
	return result;
}

void MeshState::deferPendingMesh(int idx) {
	if (idx < 0 || idx >= (int)_pendingMeshDirty.size()) {
		return;
	}
	if (!_pendingMeshDirty[idx]) {
		_pendingMeshDirty[idx] = true;
		_pendingMeshes.push(idx);
	}
}

bool MeshState::deleteMeshes(const glm::ivec3 &pos, int idx) {
	bool d = false;
	for (int i = 0; i < MeshType_Max; ++i) {
		auto &meshes = _meshes[i];
		auto iter = meshes.find(pos);
		if (iter != meshes.end()) {
			MeshState::Meshes &array = iter->value;
			if (idx < (int)array.size()) {
				voxel::Mesh *mesh = array[idx];
				delete mesh;
				array[idx] = nullptr;
			}
			d = true;
		}
	}
	return d;
}

bool MeshState::deleteMeshes(int idx) {
	bool d = false;
	for (int i = 0; i < MeshType_Max; ++i) {
		auto &meshes = _meshes[i];
		for (const auto &iter : meshes) {
			MeshState::Meshes &array = iter->value;
			if (idx < (int)array.size()) {
				voxel::Mesh *mesh = array[idx];
				delete mesh;
				array[idx] = nullptr;
			}
			d = true;
		}
	}
	return d;
}

const MeshState::MeshesMap &MeshState::meshes(MeshType type) const {
	return _meshes[type];
}

void MeshState::count(MeshType meshType, int idx, size_t &vertCount, size_t &normalsCount, size_t &indCount) const {
	for (const auto &i : _meshes[meshType]) {
		const MeshState::Meshes &meshes = i->value;
		if (idx >= (int)meshes.size()) {
			continue;
		}
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
	if (idx < 0 || idx >= (int)_volumeData.size() || !_volumeData[idx]._palette.hasValue()) {
		return voxel::getPalette();
	}
	return *_volumeData[idx]._palette.value();
}

const palette::NormalPalette &MeshState::normalsPalette(int idx) const {
	if (idx < 0 || idx >= (int)_volumeData.size() || !_volumeData[idx]._normalPalette.hasValue()) {
		static palette::NormalPalette normalPalette;
		return normalPalette;
	}
	return *_volumeData[idx]._normalPalette.value();
}

voxel::Region MeshState::calculateExtractRegion(int x, int y, int z, const glm::ivec3 &meshSize) const {
	const glm::ivec3 mins(x * meshSize.x, y * meshSize.y, z * meshSize.z);
	const glm::ivec3 maxs = mins + meshSize - 1;
	return voxel::Region{mins, maxs};
}

bool MeshState::runScheduledExtractions(size_t maxExtraction) {
	core_trace_scoped(MeshStateRunScheduledExtractions);
	const size_t n = _extractRegions.size();
	if (n == 0) {
		return false;
	}
	if (maxExtraction == 0) {
		maxExtraction = core::cpus();
	}
	ExtractRegion regions[64] {};
	if (maxExtraction > lengthof(regions)) {
		maxExtraction = lengthof(regions);
	}
	for (size_t i = 0; i < maxExtraction && i < n; ++i) {
		if (!_extractRegions.pop(regions[i])) {
			maxExtraction = i;
			break;
		}
	}
	ExtractionResult results[lengthof(regions)] {};
	Log::debug("running %i extractions in parallel", (int)maxExtraction);
	voxel::SurfaceExtractionType type = (voxel::SurfaceExtractionType)_meshMode->intVal();
	const bool meshAllocSmall = (MeshAllocStrategy)_meshAlloc->intVal() == MeshAllocStrategy::SmallGrow;
	auto fn = [&regions, &results, this, type, meshAllocSmall] (size_t start, size_t end) {
		for (size_t i = start; i < end; ++i) {
			const ExtractRegion &extractRegion = regions[i];
			const int idx = extractRegion.idx;
			if (idx == -1) {
				continue;
			}
			const voxel::RawVolume *v = volume(idx);
			if (v == nullptr) {
				continue;
			}
			if (_volumeData[idx]._generation != extractRegion.generation) {
				continue;
			}
			if (_volumeData[idx]._hidden) {
				continue;
			}
			const voxel::Region finalRegion = extractRegion.region;
			const voxel::Region copyRegion(finalRegion.getLowerCorner() - 2, finalRegion.getUpperCorner() + 2);
			if (!copyRegion.isValid()) {
				continue;
			}
			if (v->isEmpty(finalRegion)) {
				const glm::ivec3 &mins = extractRegion.region.getLowerCorner();
				results[i] = {mins, idx, voxel::ChunkMesh(0, 0, false)};
				continue;
			}

			const palette::Palette &pal = palette(resolveIdx(idx));
			const glm::ivec3 &mins = extractRegion.region.getLowerCorner();
			const int meshVertices = meshAllocSmall ? SmallAllocVertices : FullAllocVertices;
			const int meshIndices = meshAllocSmall ? SmallAllocIndices : FullAllocIndices;
			voxel::ChunkMesh mesh(meshVertices, meshIndices, true);
			voxel::SurfaceExtractionContext ctx = voxel::createContext(type, v, extractRegion.region, pal, mesh, mins);
			voxel::extractSurface(ctx);
			results[i] = {mins, idx, core::move(mesh)};
		}
	};
	app::for_parallel(0, maxExtraction, fn);

	for (size_t i = 0; i < maxExtraction; ++i) {
		ExtractionResult &result = results[i];
		if (result.idx == -1) {
			continue;
		}
		addOrReplaceMeshes(result, MeshType_Opaque);
		addOrReplaceMeshes(result, MeshType_Transparency);
		// Only enqueue once per volume index: if already dirty, the existing queue
		// entry will trigger a full re-upload that includes this new chunk too.
		if (!_pendingMeshDirty[result.idx]) {
			_pendingMeshDirty[result.idx] = true;
			_pendingMeshes.push(result.idx);
		}
	}

	return true;
}

bool MeshState::update() {
	core_trace_scoped(MeshStateUpdate);
	bool triggerClear = false;
	if (_meshMode->isDirty()) {
		_meshMode->markClean();
		clearPendingExtractions();

		for (int i : _activeIndices) {
			if (hidden(i)) {
				continue;
			}
			if (voxel::RawVolume *v = volume(i)) {
				scheduleRegionExtraction(i, v->region());
			}
		}
		triggerClear = true;
	}
	runScheduledExtractions();
	return triggerClear;
}

bool MeshState::scheduleRegionExtraction(int idx, const voxel::Region &region) {
	core_trace_scoped(MeshStateScheduleExtraction);
	const int bufferIndex = resolveIdx(idx);
	voxel::RawVolume *v = volume(bufferIndex);
	if (v == nullptr) {
		return false;
	}

	const int s = _meshSize->intVal();
	const glm::ivec3 meshSize(s);
	const glm::ivec3 meshSizeMinusOne(s - 1);
	voxel::Region completeRegion = v->region();
	completeRegion.shiftUpperCorner(1, 1, 1);

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
				_extractRegions.emplace(finalRegion, bufferIndex, _volumeData[bufferIndex]._generation);
			}
		}
	}
	return deletedMesh;
}

void MeshState::extractAllPending() {
	core_trace_scoped(MeshStateExtractAllPending);
	while (runScheduledExtractions(100)) {
	}
}

void MeshState::clearPendingExtractions() {
	core_trace_scoped(MeshStateClearPendingExtractions);
	_pendingMeshes.clear();
	_pendingMeshDirty.fill(false);
	_extractRegions.clear();
}

voxel::SurfaceExtractionType MeshState::meshMode() const {
	return (voxel::SurfaceExtractionType)_meshMode->intVal();
}

int MeshState::resolveIdx(int idx) const {
	const int ref = reference(idx);
	if (ref != -1) {
		return resolveIdx(ref);
	}
	return idx;
}

bool MeshState::sameNormalPalette(int idx, const palette::NormalPalette *palette) const {
	if (idx < 0 || idx >= (int)_volumeData.size()) {
		return false;
	}
	const palette::NormalPalette *normalPalette = _volumeData[idx]._normalPalette.value();
	if (normalPalette == nullptr) {
		return palette == nullptr;
	}
	if (palette == nullptr) {
		return false;
	}
	return normalPalette->hash() == palette->hash();
}

voxel::RawVolume *MeshState::setVolume(int idx, voxel::RawVolume *v, palette::Palette *palette, palette::NormalPalette *normalPalette, bool meshDelete,
									   bool &meshDeleted) {
	core_trace_scoped(MeshStateSetVolume);
	meshDeleted = false;
	if (idx < 0) {
		return nullptr;
	}
	ensureSize(idx);
	_volumeData[idx]._palette.setValue(palette);
	_volumeData[idx]._normalPalette.setValue(normalPalette);
	voxel::RawVolume *old = volume(idx);
	if (old == v) {
		return nullptr;
	}
	core_trace_scoped(RawVolumeRendererSetVolume);
	if (v != nullptr && old == nullptr) {
		_activeIndices.push_back(idx);
	} else if (v == nullptr && old != nullptr) {
		for (int i = 0; i < (int)_activeIndices.size(); ++i) {
			if (_activeIndices[i] == idx) {
				_activeIndices[i] = _activeIndices.back();
				_activeIndices.pop();
				break;
			}
		}
	}
	_volumeData[idx]._rawVolume = v;
	_volumeData[idx]._generation++;
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

core::Buffer<voxel::RawVolume *> MeshState::shutdown() {
	clearMeshes();
	_activeIndices.clear();
	_pendingMeshDirty.fill(false);
	core::Buffer<voxel::RawVolume *> old;
	old.reserve(_volumeData.size());
	for (int idx = 0; idx < (int)_volumeData.size(); ++idx) {
		VolumeData &state = _volumeData[idx];
		// hand over the ownership to the caller
		old.push_back(state._rawVolume);
		state._rawVolume = nullptr;
	}
	return old;
}

void MeshState::resetReferences() {
	for (int idx : _activeIndices) {
		_volumeData[idx]._reference = -1;
	}
}

int MeshState::reference(int idx) const {
	if (idx < 0 || idx >= (int)_volumeData.size()) {
		return -1;
	}
	return _volumeData[idx]._reference;
}

void MeshState::setReference(int idx, int referencedIdx) {
	if (idx < 0) {
		return;
	}
	ensureSize(idx);
	VolumeData &state = _volumeData[idx];
	state._reference = referencedIdx;
}

bool MeshState::hidden(int idx) const {
	if (idx < 0 || idx >= (int)_volumeData.size()) {
		return true;
	}
	return _volumeData[idx]._hidden;
}

void MeshState::hide(int idx, bool hide) {
	if (idx < 0) {
		return;
	}
	ensureSize(idx);
	const bool wasHidden = _volumeData[idx]._hidden;
	_volumeData[idx]._hidden = hide;
	if (wasHidden && !hide) {
		const voxel::RawVolume *v = volume(idx);
		if (v != nullptr) {
			scheduleRegionExtraction(idx, v->region());
		}
	}
}

video::Face MeshState::cullFace(int idx) const {
	if (idx < 0 || idx >= (int)_volumeData.size()) {
		return video::Face::Back;
	}
	return _volumeData[idx]._cullFace;
}

void MeshState::setCullFace(int idx, video::Face face) {
	if (idx < 0) {
		return;
	}
	ensureSize(idx);
	_volumeData[idx]._cullFace = face;
}

bool MeshState::grayed(int idx) const {
	if (idx < 0 || idx >= (int)_volumeData.size()) {
		return true;
	}
	return _volumeData[idx]._gray;
}

void MeshState::gray(int idx, bool gray) {
	if (idx < 0) {
		return;
	}
	ensureSize(idx);
	_volumeData[idx]._gray = gray;
}

bool MeshState::hasSelection(int idx) const {
	if (idx < 0 || idx >= (int)_volumeData.size()) {
		return false;
	}
	return _volumeData[idx]._hasSelection;
}

void MeshState::setHasSelection(int idx, bool hasSelection) {
	if (idx < 0) {
		return;
	}
	ensureSize(idx);
	_volumeData[idx]._hasSelection = hasSelection;
}

bool MeshState::locked(int idx) const {
	if (idx < 0 || idx >= (int)_volumeData.size()) {
		return false;
	}
	return _volumeData[idx]._locked;
}

void MeshState::setLocked(int idx, bool locked) {
	if (idx < 0) {
		return;
	}
	ensureSize(idx);
	_volumeData[idx]._locked = locked;
}

} // namespace voxel
