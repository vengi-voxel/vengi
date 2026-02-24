/**
 * @file
 */

#include "MeshState.h"
#include "app/App.h"
#include "app/Async.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/concurrent/Concurrency.h"
#include "palette/NormalPalette.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/SurfaceExtractor.h"

namespace voxel {

MeshState::MeshState() {
}

bool MeshState::init() {
	_meshMode = core::getVar(cfg::VoxRenderMeshMode);
	_meshMode->markClean();
	return true;
}

void MeshState::construct() {
	// this must be 62 for the binary cubic mesher
	_meshSize = core::Var::get(cfg::VoxelMeshSize, "62", core::CV_READONLY | core::CV_NOPERSIST);
	// Editor/render mesh mode - excludes GreedyTexture as it's not supported by the renderer
	core::Var::get(cfg::VoxRenderMeshMode, core::string::toString((int)voxel::SurfaceExtractionType::Binary),
				   core::CV_SHADER,
				   "0 = cubes, 1 = marching cubes, 2 = binary mesher",
				   core::Var::minMaxValidator<(int)voxel::SurfaceExtractionType::Cubic,
											  (int)voxel::SurfaceExtractionType::Binary>);
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
	if (idx < 0 || idx >= MAX_VOLUMES) {
		Log::error("Given id %i is out of bounds", idx);
		return false;
	}
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
	meshes.fill(nullptr);
	meshes[result.idx] = new voxel::Mesh(core::move(mesh));
	_meshes[type].emplace(result.mins, core::move(meshes));
}

int MeshState::pop() {
	int result = -1;
	_pendingMeshes.try_pop(result);
	return result;
}

bool MeshState::deleteMeshes(const glm::ivec3 &pos, int idx) {
	bool d = false;
	for (int i = 0; i < MeshType_Max; ++i) {
		auto &meshes = _meshes[i];
		auto iter = meshes.find(pos);
		if (iter != meshes.end()) {
			MeshState::Meshes &array = iter->value;
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
		for (const auto &iter : meshes) {
			MeshState::Meshes &array = iter->value;
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
	for (const auto &i : _meshes[meshType]) {
		const MeshState::Meshes &meshes = i->value;
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

const palette::NormalPalette &MeshState::normalsPalette(int idx) const {
	if (idx < 0 || idx > MAX_VOLUMES || !_volumeData[idx]._normalPalette.hasValue()) {
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
	auto fn = [&regions, &results, this, type] (size_t start, size_t end) {
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
			const voxel::Region finalRegion = extractRegion.region;
			const voxel::Region copyRegion(finalRegion.getLowerCorner() - 2, finalRegion.getUpperCorner() + 2);
			if (!copyRegion.isValid()) {
				continue;
			}

			const palette::Palette &pal = palette(resolveIdx(idx));
			const glm::ivec3 &mins = extractRegion.region.getLowerCorner();
			voxel::ChunkMesh mesh(262144, 524288, true);
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
		_pendingMeshes.push(result.idx);
	}

	return true;
}

bool MeshState::update() {
	core_trace_scoped(MeshStateUpdate);
	bool triggerClear = false;
	if (_meshMode->isDirty()) {
		_meshMode->markClean();
		clearPendingExtractions();

		for (int i = 0; i < MAX_VOLUMES; ++i) {
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
				_extractRegions.emplace(finalRegion, bufferIndex, hidden(bufferIndex));
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
	if (idx < 0 || idx >= MAX_VOLUMES) {
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
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	_volumeData[idx]._palette.setValue(palette);
	_volumeData[idx]._normalPalette.setValue(normalPalette);
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

core::Buffer<voxel::RawVolume *> MeshState::shutdown() {
	clearMeshes();
	core::Buffer<voxel::RawVolume *> old;
	old.reserve(MAX_VOLUMES);
	for (int idx = 0; idx < (int)_volumeData.size(); ++idx) {
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

video::Face MeshState::cullFace(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return video::Face::Back;
	}
	return _volumeData[idx]._cullFace;
}

void MeshState::setCullFace(int idx, video::Face face) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return;
	}
	_volumeData[idx]._cullFace = face;
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

} // namespace voxel
