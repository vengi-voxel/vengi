/**
 * @file
 */

#include "ViewportSingleton.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/VolumeCropper.h"
#include "voxel/polyvox/VolumeRotator.h"
#include "voxel/polyvox/VolumeMover.h"
#include "voxel/polyvox/VolumeRescaler.h"
#include "voxel/polyvox/VolumeVisitor.h"
#include "voxel/polyvox//RawVolumeWrapper.h"
#include "voxel/polyvox//RawVolumeMoveWrapper.h"
#include "voxel/generator/CloudGenerator.h"
#include "voxel/generator/CactusGenerator.h"
#include "voxel/generator/BuildingGenerator.h"
#include "voxel/generator/PlantGenerator.h"
#include "voxel/generator/TreeGenerator.h"
#include "voxel/BiomeManager.h"
#include "voxelformat/VoxFormat.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/QBFormat.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedBlendMode.h"
#include "math/Random.h"
#include "core/Array.h"
#include "core/App.h"
#include "core/Log.h"
#include "io/Filesystem.h"
#include "voxedit-util/tool/Crop.h"
#include "voxedit-util/tool/Expand.h"
#include "voxedit-util/tool/Fill.h"
#include "voxedit-util/ImportHeightmap.h"
#include "core/GLM.h"
#include <set>

#define VOXELIZER_IMPLEMENTATION
#include "voxelizer.h"

namespace voxedit {

const int leafSize = 8;

ViewportSingleton::ViewportSingleton() :
		_gridRenderer(true, true) {
}

ViewportSingleton::~ViewportSingleton() {
	shutdown();
}

bool ViewportSingleton::exportModel(const std::string& file) {
	core_trace_scoped(EditorSceneExportModel);
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file), io::FileMode::Write);
	if (!(bool)filePtr) {
		return false;
	}
	voxel::Mesh mesh(128, 128, true);
	vps().volumeRenderer().toMesh(ModelVolumeIndex, &mesh);
	return voxel::exportMesh(&mesh, filePtr->name().c_str());
}

bool ViewportSingleton::voxelizeModel(const video::MeshPtr& meshPtr) {
	const video::Mesh::Vertices& positions = meshPtr->vertices();
	const video::Mesh::Indices& indices = meshPtr->indices();

	if (indices.size() < 8) {
		Log::error("Not enough indices found: %i", (int)indices.size());
		return false;
	}

	vx_mesh_t* mesh = vx_color_mesh_alloc(positions.size(), indices.size());
	if (mesh == nullptr) {
		Log::error("Failed to allocate voxelize mesh");
		return false;
	}

	for (size_t f = 0; f < mesh->nindices; f++) {
		mesh->indices[f] = indices[f];
		mesh->normalindices[f] = indices[f];
	}

	for (size_t v = 0u; v < mesh->nvertices; ++v) {
		const video::Mesh::Vertices::value_type& vertex = positions[v];
		mesh->vertices[v].x = vertex._pos.x;
		mesh->vertices[v].y = vertex._pos.y;
		mesh->vertices[v].z = vertex._pos.z;
		mesh->normals[v].x = vertex._norm.x;
		mesh->normals[v].y = vertex._norm.y;
		mesh->normals[v].z = vertex._norm.z;
		mesh->colors[v].x = vertex._color.x;
		mesh->colors[v].y = vertex._color.y;
		mesh->colors[v].z = vertex._color.z;
	}

	const glm::vec3& meshMins = meshPtr->mins();
	const glm::vec3& meshMaxs = meshPtr->maxs();
	const glm::vec3& meshDimension = meshMaxs - meshMins;

	const voxel::RawVolume* model = modelVolume();
	const voxel::Region& region = model->region();
	const glm::vec3 regionDimension(region.getDimensionsInCells());
	const glm::vec3 factor = regionDimension / meshDimension;
	Log::debug("%f:%f:%f", factor.x, factor.y, factor.z);

	const float voxelSize = glm::min(glm::min(factor.x, factor.y), factor.z);
	const float precision = voxelSize / 10.0f;
	vx_point_cloud_t* result = vx_voxelize_pc(mesh, voxelSize, voxelSize, voxelSize, precision);
	Log::debug("Number of vertices: %i", (int)result->nvertices);

	for (size_t i = 0u; i < result->nvertices; ++i) {
		result->vertices[i].x -= meshMins.x;
		result->vertices[i].y -= meshMins.y;
		result->vertices[i].z -= meshMins.z;
	}
	pointCloud((const glm::vec3*)result->vertices, (const glm::vec3*)result->colors, result->nvertices);

	vx_point_cloud_free(result);
	vx_mesh_free(mesh);

	return true;
}

bool ViewportSingleton::importHeightmap(const std::string& file) {
	voxel::RawVolume* v = modelVolume();
	if (v == nullptr) {
		return false;
	}
	const image::ImagePtr& img = image::loadImage(file, false);
	if (!img->isLoaded()) {
		return false;
	}
	voxel::RawVolumeWrapper wrapper(v);
	voxedit::importHeightmap(wrapper, img);
	modified(wrapper.dirtyRegion());
	return true;
}

bool ViewportSingleton::save(const std::string& file) {
	if (modelVolume() == nullptr) {
		return false;
	}
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file), io::FileMode::Write);
	if (filePtr->extension() == "qbt") {
		voxel::QBTFormat f;
		if (f.save(modelVolume(), filePtr)) {
			_dirty = false;
			return true;
		}
	} else if (filePtr->extension() == "vox") {
		voxel::VoxFormat f;
		if (f.save(modelVolume(), filePtr)) {
			_dirty = false;
			return true;
		}
	} else if (filePtr->extension() == "qb") {
		voxel::QBFormat f;
		if (f.save(modelVolume(), filePtr)) {
			_dirty = false;
			return true;
		}
	}
	return false;
}

bool ViewportSingleton::prefab(const std::string& file) {
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(file);
	if (!(bool)filePtr) {
		Log::error("Failed to open model file %s", file.data());
		return false;
	}
	voxel::RawVolume* newVolume;

	if (filePtr->extension() == "qbt") {
		voxel::QBTFormat f;
		newVolume = f.load(filePtr);
	} else if (filePtr->extension() == "vox") {
		voxel::VoxFormat f;
		newVolume = f.load(filePtr);
	} else if (filePtr->extension() == "qb") {
		voxel::QBFormat f;
		newVolume = f.load(filePtr);
	} else {
		newVolume = nullptr;
	}
	if (newVolume == nullptr) {
		Log::error("Failed to load model file %s", file.c_str());
		return false;
	}
	Log::info("Import model file %s", file.c_str());
	voxel::RawVolumeMoveWrapper wrapper(modelVolume());
	voxel::moveVolume(&wrapper, newVolume, _referencePos);
	modified(newVolume->region());
	delete newVolume;
	return true;
}

bool ViewportSingleton::load(const std::string& file) {
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(file);
	if (!(bool)filePtr) {
		Log::error("Failed to open model file %s", file.data());
		return false;
	}
	voxel::RawVolume* newVolume;

	if (filePtr->extension() == "qbt") {
		voxel::QBTFormat f;
		newVolume = f.load(filePtr);
	} else if (filePtr->extension() == "vox") {
		voxel::VoxFormat f;
		newVolume = f.load(filePtr);
	} else if (filePtr->extension() == "qb") {
		voxel::QBFormat f;
		newVolume = f.load(filePtr);
	} else {
		newVolume = nullptr;
	}
	if (newVolume == nullptr) {
		Log::error("Failed to load model file %s", file.c_str());
		return false;
	}
	Log::info("Load model file %s", file.c_str());
	mementoHandler().clearStates();
	setNewVolume(newVolume);
	modified(newVolume->region());
	_dirty = false;
	return true;
}

void ViewportSingleton::setMousePos(int x, int y) {
	_mouseX = x;
	_mouseY = y;
}

void ViewportSingleton::modified(const voxel::Region& modifiedRegion, bool markUndo) {
	if (!modifiedRegion.isValid()) {
		return;
	}
	if (markUndo) {
		mementoHandler().markUndo(modelVolume());
	}
	_extractRegions.push_back(modifiedRegion);
	_dirty = true;
	_extract = true;
	resetLastTrace();
}

void ViewportSingleton::crop() {
	if (_empty) {
		Log::info("Empty volumes can't be cropped");
		return;
	}
	voxel::RawVolume* newVolume = voxedit::tool::crop(modelVolume());
	if (newVolume == nullptr) {
		return;
	}
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void ViewportSingleton::extend(const glm::ivec3& size) {
	voxel::RawVolume* newVolume = voxedit::tool::expand(modelVolume(), size);
	if (newVolume == nullptr) {
		return;
	}
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void ViewportSingleton::scaleHalf() {
	// TODO: check that src region boundaries are even
	const voxel::Region& srcRegion = modelVolume()->region();
	const int w = srcRegion.getWidthInVoxels();
	const int h = srcRegion.getHeightInVoxels();
	const int d = srcRegion.getDepthInVoxels();
	const glm::ivec3 maxs(w / 2, h / 2, d / 2);
	voxel::Region region(glm::zero<glm::ivec3>(), maxs);
	voxel::RawVolume* newVolume = new voxel::RawVolume(region);
	voxel::RawVolumeWrapper wrapper(newVolume);
	voxel::rescaleVolume(*modelVolume(), *newVolume);
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void ViewportSingleton::pointCloud(const glm::vec3* vertices, const glm::vec3 *vertexColors, size_t amount) {
	glm::ivec3 mins(std::numeric_limits<glm::ivec3::value_type>::max());
	glm::ivec3 maxs(std::numeric_limits<glm::ivec3::value_type>::min());

	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	materialColors.erase(materialColors.begin());
	voxel::RawVolumeWrapper wrapper(modelVolume());

	bool change = false;
	for (size_t idx = 0u; idx < amount; ++idx) {
		const glm::vec3& vertex = vertices[idx];
		const glm::vec3& color = vertexColors[idx];
		const glm::ivec3 pos(_cursorPos.x + vertex.x, _cursorPos.y + vertex.y, _cursorPos.z + vertex.z);
		const glm::vec4 cvec(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, 255.0f);
		const uint8_t index = core::Color::getClosestMatch(cvec, materialColors);
		if (wrapper.setVoxel(pos, voxel::createVoxel(voxel::VoxelType::Generic, index))) {
			mins = glm::min(mins, pos);
			maxs = glm::max(maxs, pos);
			change = true;
		}
	}
	if (!change) {
		return;
	}
	const voxel::Region modifiedRegion(mins, maxs);
	modified(modifiedRegion);
}

bool ViewportSingleton::aabbMode() const {
	return _aabbMode;
}

glm::ivec3 ViewportSingleton::aabbDim() const {
	return glm::abs(cursorPosition() - _aabbFirstPos);
}

bool ViewportSingleton::aabbStart() {
	if (_aabbMode) {
		return false;
	}
	_aabbFirstPos = cursorPosition();
	_aabbMode = true;
	return true;
}

bool ViewportSingleton::getMirrorAABB(glm::ivec3& mins, glm::ivec3& maxs) const {
	if (_mirrorAxis == math::Axis::None) {
		return false;
	}
	const int index = getIndexForMirrorAxis(_mirrorAxis);
	int deltaMaxs = _mirrorPos[index] - maxs[index] - 1;
	deltaMaxs *= 2;
	deltaMaxs += (maxs[index] - mins[index] + 1);
	mins[index] += deltaMaxs;
	maxs[index] += deltaMaxs;
	return true;
}

bool ViewportSingleton::aabbEnd() {
	if (!_aabbMode) {
		return false;
	}
	voxel::RawVolumeWrapper wrapper(modelVolume());
	_aabbMode = false;
	const glm::ivec3& pos = cursorPosition();
	const glm::ivec3 mins = glm::min(_aabbFirstPos, pos);
	const glm::ivec3 maxs = glm::max(_aabbFirstPos, pos);
	const bool deleteVoxels = (_modifierType & ModifierType::Delete) == ModifierType::Delete;
	const bool overwriteVoxels = (_modifierType & ModifierType::Place) == ModifierType::Place && deleteVoxels;
	const voxel::Voxel voxel = (deleteVoxels && !overwriteVoxels) ? voxel::createVoxel(voxel::VoxelType::Air, 0) : _cursorVoxel;
	voxel::Region modifiedRegion;
	glm::ivec3 minsMirror = mins;
	glm::ivec3 maxsMirror = maxs;
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		if (voxedit::tool::aabb(wrapper, mins, maxs, voxel, overwriteVoxels || deleteVoxels, &modifiedRegion)) {
			modified(modifiedRegion);
		}
		return true;
	}
	const math::AABB<int> first(mins, maxs);
	const math::AABB<int> second(minsMirror, maxsMirror);
	voxel::Region modifiedRegionMirror;
	bool success;
	if (math::intersects(first, second)) {
		if (voxedit::tool::aabb(wrapper, mins, maxsMirror, voxel, overwriteVoxels || deleteVoxels, &modifiedRegionMirror)) {
			modified(modifiedRegionMirror);
		}
	} else {
		if (voxedit::tool::aabb(wrapper, mins, maxs, voxel, overwriteVoxels || deleteVoxels, &modifiedRegion)) {
			modified(modifiedRegion);
		}
		if (voxedit::tool::aabb(wrapper, minsMirror, maxsMirror, voxel, overwriteVoxels || deleteVoxels, &modifiedRegionMirror)) {
			modified(modifiedRegionMirror);
		}
	}
	return true;
}

void ViewportSingleton::undo() {
	voxel::RawVolume* v = mementoHandler().undo();
	if (v == nullptr) {
		return;
	}
	setNewVolume(v);
	modified(v->region(), false);
}

void ViewportSingleton::redo() {
	voxel::RawVolume* v = mementoHandler().redo();
	if (v == nullptr) {
		return;
	}
	setNewVolume(v);
	modified(v->region(), false);
}

void ViewportSingleton::resetLastTrace() {
	_lastRaytraceX = _lastRaytraceY = -1;
}

void ViewportSingleton::setNewVolume(voxel::RawVolume* volume) {
	const voxel::Region& region = volume->region();

	delete _volumeRenderer.setVolume(ModelVolumeIndex, volume);

#if 0
	if (_spaceColonizationTree != nullptr) {
		delete _spaceColonizationTree;
		_spaceColonizationTree = nullptr;
	}
#endif

	if (volume != nullptr) {
		const voxel::Region& region = volume->region();
		_gridRenderer.update(region);
	} else {
		_gridRenderer.clear();
	}

	_dirty = false;
	_result = voxel::PickResult();
	const glm::ivec3 pos = _cursorPos;
	_cursorPos = pos * 10 + 10;
	setCursorPosition(pos);
	setReferencePosition(region.getCentre());
	resetLastTrace();
}

bool ViewportSingleton::newVolume(bool force) {
	if (dirty() && !force) {
		return false;
	}
	const voxel::Region region(glm::ivec3(0), glm::ivec3(size() - 1));
	mementoHandler().clearStates();
	setNewVolume(new voxel::RawVolume(region));
	modified(region);
	_dirty = false;
	return true;
}

void ViewportSingleton::rotate(int angleX, int angleY, int angleZ) {
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = voxel::rotateVolume(model, glm::vec3(angleX, angleY, angleZ), voxel::Voxel(), false);
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void ViewportSingleton::move(int x, int y, int z) {
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = new voxel::RawVolume(model->region());
	voxel::RawVolumeMoveWrapper wrapper(newVolume);
	voxel::moveVolume(&wrapper, model, glm::ivec3(x, y, z));
	setNewVolume(newVolume);
	modified(newVolume->region());
}

const voxel::Voxel& ViewportSingleton::getVoxel(const glm::ivec3& pos) const {
	return modelVolume()->voxel(pos);
}

void ViewportSingleton::render(const video::Camera& camera) {
	const bool depthTest = video::enable(video::State::DepthTest);
	_empty = _volumeRenderer.empty(ModelVolumeIndex);
	_gridRenderer.render(camera, modelVolume()->region());
	_volumeRenderer.render(camera, _renderShadow);
	if (_aabbMode) {
		_shapeBuilder.clear();
		_shapeBuilder.setColor(core::Color::alpha(core::Color::Red, 0.5f));
		glm::ivec3 cursor = cursorPosition();
		glm::ivec3 mins = glm::min(_aabbFirstPos, cursor);
		glm::ivec3 maxs = glm::max(_aabbFirstPos, cursor);
		glm::ivec3 minsMirror = mins;
		glm::ivec3 maxsMirror = maxs;
		if (getMirrorAABB(minsMirror, maxsMirror)) {
			const math::AABB<int> first(mins, maxs);
			const math::AABB<int> second(minsMirror, maxsMirror);
			if (math::intersects(first, second)) {
				_shapeBuilder.cube(glm::vec3(mins) - 0.001f, glm::vec3(maxsMirror) + 1.001f);
			} else {
				_shapeBuilder.cube(glm::vec3(mins) - 0.001f, glm::vec3(maxs) + 1.001f);
				_shapeBuilder.cube(glm::vec3(minsMirror) - 0.001f, glm::vec3(maxsMirror) + 1.001f);
			}
		} else {
			_shapeBuilder.cube(glm::vec3(mins) - 0.001f, glm::vec3(maxs) + 1.001f);
		}
		_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
		_shapeRenderer.render(_aabbMeshIndex, camera);
	}
	_shapeRenderer.render(_voxelCursorMesh, camera, glm::translate(glm::vec3(cursorPosition())));
	// TODO: render error if rendered last - but be before grid renderer to get transparency.
	if (_renderLockAxis) {
		for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
			_shapeRenderer.render(_planeMeshIndex[i], camera);
		}
	}
	_shapeRenderer.render(_mirrorMeshIndex, camera);
	if (renderAxis()) {
		_axis.render(camera);
	}
	if (!depthTest) {
		video::disable(video::State::DepthTest);
	}
	_shapeRenderer.render(_referencePointMesh, camera);
}

bool ViewportSingleton::init() {
	++_initialized;
	if (_initialized > 1) {
		return true;
	}
	_axis.init();
	_volumeRenderer.construct();
	//_volumeRenderer.setAmbientColor(glm::vec3(core::Color::White));
	_volumeRenderer.init();
	_shapeRenderer.init();
	_gridRenderer.init();

	_mirrorMeshIndex = -1;
	_aabbMeshIndex = -1;
	for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
		_planeMeshIndex[i] = -1;
	}

	_lockedAxis = math::Axis::None;
	_mirrorAxis = math::Axis::None;
	return true;
}

void ViewportSingleton::update() {
	const uint64_t ms = core::App::getInstance()->systemMillis();
	if (_spaceColonizationTree != nullptr && ms - _lastGrow > 1000L) {
		const bool growing = _spaceColonizationTree->step();
		_lastGrow = ms;
		voxel::RawVolumeWrapper wrapper(modelVolume());
		math::Random random;
		const voxel::RandomVoxel woodRandomVoxel(voxel::VoxelType::Wood, random);
		_spaceColonizationTree->generate(wrapper, woodRandomVoxel);
		modified(wrapper.dirtyRegion());
		if (!growing) {
			Log::info("done with growing the tree");
			const voxel::RandomVoxel leavesRandomVoxel(voxel::VoxelType::Leaf, random);
			_spaceColonizationTree->generateLeaves(wrapper, leavesRandomVoxel, glm::ivec3(leafSize));
			delete _spaceColonizationTree;
			_spaceColonizationTree = nullptr;
		}
	}

	extractVolume();
}

void ViewportSingleton::shutdown() {
	--_initialized;
	if (_initialized != 0) {
		return;
	}
	std::vector<voxel::RawVolume*> old = _volumeRenderer.shutdown();
	for (voxel::RawVolume* v : old) {
		delete v;
	}

	if (_spaceColonizationTree != nullptr) {
		delete _spaceColonizationTree;
		_spaceColonizationTree = nullptr;
	}

	_axis.shutdown();
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_gridRenderer.shutdown();
	mementoHandler().clearStates();
}

bool ViewportSingleton::extractVolume() {
	if (_extract) {
		Log::debug("Extract the mesh");
		_extract = false;
		for (const voxel::Region& region : _extractRegions) {
			if (!_volumeRenderer.extract(ModelVolumeIndex, region)) {
				Log::error("Failed to extract the model mesh");
			}
		}
		_extractRegions.clear();
		return true;
	}
	return false;
}

void ViewportSingleton::noise(int octaves, float lacunarity, float frequency, float gain, voxel::noisegen::NoiseType type) {
	math::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::noisegen::generate(wrapper, octaves, lacunarity, frequency, gain, type, random);
	modified(wrapper.dirtyRegion());
}

void ViewportSingleton::spaceColonization() {
	if (_spaceColonizationTree) {
		return;
	}
	const voxel::Region& region = modelVolume()->region();
	const math::AABB<int>& aabb = region.aabb();
	const int trunkHeight = aabb.getWidthY() / 3;
	_lastGrow = core::App::getInstance()->systemMillis();

	const int branchLength = 6;
	const float branchSize = 4.0f;
	Log::info("Create spacecolonization tree with branch length %i, branch size %f, trunk height: %i, leaf size: %i",
			branchLength, branchSize, trunkHeight, leafSize);
	_spaceColonizationTree = new voxel::tree::Tree(referencePosition(), trunkHeight, branchLength,
			aabb.getWidthX() - leafSize, aabb.getWidthY() - trunkHeight - leafSize, aabb.getWidthZ() - leafSize, branchSize, _lastGrow);
}

void ViewportSingleton::createCactus() {
	math::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::cactus::createCactus(wrapper, _referencePos, 18, 2, random);
	modified(wrapper.dirtyRegion());
}

void ViewportSingleton::createCloud() {
	voxel::RawVolumeWrapper wrapper(modelVolume());
	struct HasClouds {
		glm::vec2 pos;
		void getCloudPositions(const voxel::Region& region, std::vector<glm::vec2>& positions, math::Random& random, int border) const {
			positions.push_back(pos);
		}
	};
	HasClouds hasClouds;
	hasClouds.pos = glm::vec2(_referencePos.x, _referencePos.z);
	voxel::cloud::CloudContext cloudCtx;
	if (voxel::cloud::createClouds(wrapper, wrapper.region(), hasClouds, cloudCtx)) {
		modified(modelVolume()->region());
	}
}

void ViewportSingleton::createPlant(voxel::PlantType type) {
	voxel::PlantGenerator g;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	if (type == voxel::PlantType::Flower) {
		Log::info("create flower");
		g.createFlower(5, _referencePos, wrapper);
	} else if (type == voxel::PlantType::Grass) {
		Log::info("create grass");
		g.createGrass(10, _referencePos, wrapper);
	} else if (type == voxel::PlantType::Mushroom) {
		Log::info("create mushroom");
		g.createMushroom(7, _referencePos, wrapper);
	}
	g.shutdown();
	modified(wrapper.dirtyRegion());
}

void ViewportSingleton::createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx) {
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::building::createBuilding(wrapper, _referencePos, type);
	modified(wrapper.dirtyRegion());
}

void ViewportSingleton::createTree(voxel::TreeContext ctx) {
	math::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	ctx.pos = _referencePos;
	voxel::tree::createTree(wrapper, ctx, random);
	modified(wrapper.dirtyRegion());
}

void ViewportSingleton::setCursorVoxel(const voxel::Voxel& voxel) {
	_cursorVoxel = voxel;
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(voxel::getMaterialColor(voxel), 0.7f));
	_shapeBuilder.setPosition(glm::zero<glm::vec3>());
	_shapeBuilder.cube(glm::vec3(-0.01f), glm::vec3(1.01f));
	_shapeRenderer.createOrUpdate(_voxelCursorMesh, _shapeBuilder);
}

void ViewportSingleton::setReferencePosition(const glm::ivec3& pos) {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(core::Color::SteelBlue, 0.8f));
	const glm::vec3 posalgined{pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f};
	_shapeBuilder.setPosition(posalgined);
	_shapeBuilder.sphere(8, 6, 0.5f);
	_shapeRenderer.createOrUpdate(_referencePointMesh, _shapeBuilder);
	_referencePos = pos;
}

void ViewportSingleton::setCursorPosition(glm::ivec3 pos, bool force) {
	if (!force) {
		if ((_lockedAxis & math::Axis::X) != math::Axis::None) {
			pos.x = _cursorPos.x;
		}
		if ((_lockedAxis & math::Axis::Y) != math::Axis::None) {
			pos.y = _cursorPos.y;
		}
		if ((_lockedAxis & math::Axis::Z) != math::Axis::None) {
			pos.z = _cursorPos.z;
		}
	}

	const voxel::Region& region = modelVolume()->region();
	if (!region.containsPoint(pos)) {
		pos = region.moveInto(pos.x, pos.y, pos.z);
	}
	if (_cursorPos == pos) {
		return;
	}
	_cursorPos = pos;

	updateLockedPlane(math::Axis::X);
	updateLockedPlane(math::Axis::Y);
	updateLockedPlane(math::Axis::Z);
}

bool ViewportSingleton::renderAxis() const {
	return _renderAxis;
}

void ViewportSingleton::setRenderAxis(bool renderAxis) {
	_renderAxis = renderAxis;
}

bool ViewportSingleton::renderLockAxis() const {
	return _renderLockAxis;
}

void ViewportSingleton::setRenderLockAxis(bool renderLockAxis) {
	_renderLockAxis = renderLockAxis;
}

bool ViewportSingleton::renderShadow() const {
	return _renderShadow;
}

void ViewportSingleton::setRenderShadow(bool shadow) {
	_renderShadow = shadow;
	Log::info("render shadow: %i", shadow ? 1 : 0);
}

bool ViewportSingleton::addModifierType(ModifierType type) {
	if ((_modifierType & type) == type) {
		return false;
	}
	_modifierType &= type;
	return true;
}

void ViewportSingleton::setModifierType(ModifierType type) {
	_modifierType = type;
	// the modifier type has an influence on which voxel is taken. So make
	// sure the next trace is executed even if we don't move the mouse.
	resetLastTrace();
}

ModifierType ViewportSingleton::modifierType() const {
	return _modifierType;
}

bool ViewportSingleton::modifierTypeRequiresExistingVoxel() const {
	return (_modifierType & ModifierType::Delete) == ModifierType::Delete;
}

bool ViewportSingleton::trace(const video::Camera& camera, bool force) {
	if (modelVolume() == nullptr) {
		return false;
	}

	if (_lastRaytraceX != _mouseX || _lastRaytraceY != _mouseY || force) {
		core_trace_scoped(EditorSceneOnProcessUpdateRay);
		_lastRaytraceX = _mouseX;
		_lastRaytraceY = _mouseY;

		const video::Ray& ray = camera.mouseRay(glm::ivec2(_mouseX, _mouseY));
		const glm::vec3& dirWithLength = ray.direction * camera.farPlane();
		static constexpr voxel::Voxel air;
		_result = voxel::pickVoxel(modelVolume(), ray.origin, dirWithLength, air);

		if (modifierTypeRequiresExistingVoxel()) {
			if (_result.didHit) {
				setCursorPosition(_result.hitVoxel);
			} else if (_result.validPreviousPosition) {
				setCursorPosition(_result.previousPosition);
			}
		} else if (_result.validPreviousPosition) {
			setCursorPosition(_result.previousPosition);
		} else if (_result.didHit) {
			setCursorPosition(_result.hitVoxel);
		}
	}

	return true;
}

int ViewportSingleton::getIndexForAxis(math::Axis axis) const {
	if (axis == math::Axis::X) {
		return 0;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 2;
}

int ViewportSingleton::getIndexForMirrorAxis(math::Axis axis) const {
	if (axis == math::Axis::X) {
		return 2;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 0;
}

void ViewportSingleton::updateShapeBuilderForPlane(bool mirror, const glm::ivec3& pos, math::Axis axis, const glm::vec4& color) {
	const voxel::Region& region = modelVolume()->region();
	const int index = mirror ? getIndexForMirrorAxis(axis) : getIndexForAxis(axis);
	glm::vec3 mins = region.getLowerCorner();
	glm::vec3 maxs = region.getUpperCorner();
	mins[index] = maxs[index] = pos[index];
	const glm::vec3& ll = mins;
	const glm::vec3& ur = maxs;
	glm::vec3 ul;
	glm::vec3 lr;
	if (axis == math::Axis::Y) {
		ul = glm::vec3(mins.x, mins.y, maxs.z);
		lr = glm::vec3(maxs.x, maxs.y, mins.z);
	} else {
		ul = glm::vec3(mins.x, maxs.y, mins.z);
		lr = glm::vec3(maxs.x, mins.y, maxs.z);
	}
	std::vector<glm::vec3> vecs({ll, ul, ur, lr});
	// lower left (0), upper left (1), upper right (2)
	// lower left (0), upper right (2), lower right (3)
	const std::vector<uint32_t> indices { 0, 1, 2, 0, 2, 3, 2, 1, 0, 3, 2, 0 };
	_shapeBuilder.clear();
	_shapeBuilder.setColor(color);
	_shapeBuilder.geom(vecs, indices);
}

void ViewportSingleton::updateLockedPlane(math::Axis axis) {
	if (axis == math::Axis::None) {
		return;
	}
	const int index = getIndexForAxis(axis);
	int32_t& meshIndex = _planeMeshIndex[index];
	if ((_lockedAxis & axis) == math::Axis::None) {
		if (meshIndex != -1) {
			_shapeRenderer.deleteMesh(meshIndex);
			meshIndex = -1;
		}
		return;
	}

	const glm::vec4 colors[] = {
		core::Color::LightRed,
		core::Color::LightGreen,
		core::Color::LightBlue
	};
	updateShapeBuilderForPlane(false, _cursorPos, axis, core::Color::alpha(colors[index], 0.4f));
	_shapeRenderer.createOrUpdate(meshIndex, _shapeBuilder);
}

math::Axis ViewportSingleton::mirrorAxis() const {
	return _mirrorAxis;
}

void ViewportSingleton::setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos) {
	if (_mirrorAxis == axis) {
		if (_mirrorPos != mirrorPos) {
			_mirrorPos = mirrorPos;
			updateMirrorPlane();
		}
		return;
	}
	_mirrorPos = mirrorPos;
	_mirrorAxis = axis;
	updateMirrorPlane();
}

void ViewportSingleton::updateMirrorPlane() {
	if (_mirrorAxis == math::Axis::None) {
		if (_mirrorMeshIndex != -1) {
			_shapeRenderer.deleteMesh(_mirrorMeshIndex);
			_mirrorMeshIndex = -1;
		}
		return;
	}

	updateShapeBuilderForPlane(true, _mirrorPos, _mirrorAxis, core::Color::alpha(core::Color::LightGray, 0.3f));
	_shapeRenderer.createOrUpdate(_mirrorMeshIndex, _shapeBuilder);
}

void ViewportSingleton::setLockedAxis(math::Axis axis, bool unlock) {
	if (unlock) {
		_lockedAxis &= ~axis;
	} else {
		_lockedAxis |= axis;
	}
	updateLockedPlane(math::Axis::X);
	updateLockedPlane(math::Axis::Y);
	updateLockedPlane(math::Axis::Z);
}

}
