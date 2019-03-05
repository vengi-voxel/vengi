/**
 * @file
 */

#include "SceneManager.h"
#include "VoxEdit.h"

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
#include "voxelformat/MeshExporter.h"
#include "voxelformat/VoxFormat.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/QBFormat.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedBlendMode.h"
#include "math/Random.h"
#include "core/command/Command.h"
#include "core/Array.h"
#include "core/App.h"
#include "core/Log.h"
#include "io/Filesystem.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/tool/Crop.h"
#include "voxedit-util/tool/Expand.h"
#include "voxedit-util/tool/Fill.h"
#include "voxedit-util/ImportHeightmap.h"
#include "core/GLM.h"
#include <set>

#define VOXELIZER_IMPLEMENTATION
#include "voxedit-util/voxelizer.h"

namespace voxedit {

const int leafSize = 8;

SceneManager& sceneMgr() {
	VoxEdit* voxedit = (VoxEdit*)video::WindowedApp::getInstance();
	return voxedit->sceneMgr();
}

SceneManager::SceneManager() :
		_gridRenderer(true, true) {
}

SceneManager::~SceneManager() {
	shutdown();
}

bool SceneManager::exportModel(const std::string& file) {
	core_trace_scoped(EditorSceneExportModel);
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file), io::FileMode::Write);
	if (!(bool)filePtr) {
		return false;
	}
	voxel::Mesh mesh(128, 128, true);
	_volumeRenderer.toMesh(ModelVolumeIndex, &mesh);
	return voxel::exportMesh(&mesh, filePtr->name().c_str());
}

voxel::Region SceneManager::region() const {
	const voxel::RawVolume* volume = _volumeRenderer.volume(ModelVolumeIndex);
	if (volume == nullptr) {
		return voxel::Region();
	}
	return volume->region();
}

bool SceneManager::voxelizeModel(const video::MeshPtr& meshPtr) {
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

bool SceneManager::importHeightmap(const std::string& file) {
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

void SceneManager::autosave() {
	if (!_needAutoSave) {
		return;
	}
	const core::TimeProviderPtr& timeProvider = core::App::getInstance()->timeProvider();
	const uint64_t delay = _autoSaveSecondsDelay->intVal();
	if (_lastAutoSave + delay > timeProvider->tickSeconds()) {
		return;
	}
	std::string autoSaveFilename;
	if (_lastFilename.empty()) {
		autoSaveFilename = "autosave.vox";
	} else {
		autoSaveFilename = "autosave-" + _lastFilename;
	}
	if (save(autoSaveFilename, true)) {
		Log::info("Autosave file %s", autoSaveFilename.c_str());
	} else {
		Log::warn("Failed to autosave");
	}
	_lastAutoSave = timeProvider->tickSeconds();
}

bool SceneManager::save(const std::string& file, bool autosave) {
	if (modelVolume() == nullptr) {
		return false;
	}
	if (file.empty()) {
		Log::warn("No filename given for saving");
		return false;
	}
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(file, io::FileMode::Write);
	bool saved = false;
	std::string ext = filePtr->extension();
	if (ext.empty()) {
		Log::warn("No file extension given for saving, assuming vox");
		ext = "vox";
	}
	if (ext == "qbt") {
		voxel::QBTFormat f;
		saved = f.save(modelVolume(), filePtr);
	} else if (ext == "vox") {
		voxel::VoxFormat f;
		saved = f.save(modelVolume(), filePtr);
	} else if (ext == "qb") {
		voxel::QBFormat f;
		saved = f.save(modelVolume(), filePtr);
	} else {
		Log::warn("Failed to save file with unknown type: %s", ext.c_str());
	}
	if (saved) {
		if (!autosave) {
			_dirty = false;
			_lastFilename = file;
		}
		core::Var::get(cfg::VoxEditLastFile)->setVal(file);
		_needAutoSave = false;
	}
	return saved;
}

bool SceneManager::prefab(const std::string& file) {
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

bool SceneManager::load(const std::string& file) {
	if (file.empty()) {
		return false;
	}
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(file);
	if (!(bool)filePtr) {
		Log::error("Failed to open model file '%s'", file.data());
		return false;
	}
	voxel::RawVolume* newVolume;

	const std::string& ext = filePtr->extension();
	_lastFilename = filePtr->fileName() + "." + ext;
	if (ext == "qbt") {
		voxel::QBTFormat f;
		newVolume = f.load(filePtr);
	} else if (ext == "vox") {
		voxel::VoxFormat f;
		newVolume = f.load(filePtr);
	} else if (ext == "qb") {
		voxel::QBFormat f;
		newVolume = f.load(filePtr);
	} else {
		Log::error("Failed to load model file %s - unsupported file format", file.c_str());
		return false;
	}
	if (newVolume == nullptr) {
		Log::error("Failed to load model file %s", file.c_str());
		return false;
	}
	Log::info("Load model file %s", file.c_str());
	_mementoHandler.clearStates();
	setNewVolume(newVolume);
	modified(newVolume->region());
	_dirty = false;
	return true;
}

void SceneManager::setMousePos(int x, int y) {
	_mouseX = x;
	_mouseY = y;
}

void SceneManager::modified(const voxel::Region& modifiedRegion, bool markUndo) {
	if (!modifiedRegion.isValid()) {
		return;
	}
	if (markUndo) {
		_mementoHandler.markUndo(modelVolume());
	}
	_extractRegions.push_back(modifiedRegion);
	_dirty = true;
	_needAutoSave = true;
	_extract = true;
	resetLastTrace();
}

void SceneManager::crop() {
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

void SceneManager::extend(const glm::ivec3& size) {
	voxel::RawVolume* newVolume = voxedit::tool::expand(modelVolume(), size);
	if (newVolume == nullptr) {
		return;
	}
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void SceneManager::scaleHalf() {
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

void SceneManager::pointCloud(const glm::vec3* vertices, const glm::vec3 *vertexColors, size_t amount) {
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

bool SceneManager::aabbMode() const {
	return _aabbMode;
}

glm::ivec3 SceneManager::aabbDim() const {
	const int size = gridResolution();
	const glm::ivec3& pos = cursorPosition();
	const glm::ivec3 mins = glm::min(_aabbFirstPos, pos);
	const glm::ivec3 maxs = glm::max(_aabbFirstPos, pos);
	return glm::abs(maxs + size - mins);
}

bool SceneManager::aabbStart() {
	if (_aabbMode) {
		return false;
	}
	_aabbFirstPos = cursorPosition();
	_aabbMode = true;
	return true;
}

bool SceneManager::getMirrorAABB(glm::ivec3& mins, glm::ivec3& maxs) const {
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

bool SceneManager::aabbEnd() {
	if (!_aabbMode) {
		return false;
	}
	voxel::RawVolumeWrapper wrapper(modelVolume());
	_aabbMode = false;
	const int size = gridResolution();
	const glm::ivec3 pos = cursorPosition();
	const glm::ivec3 mins = glm::min(_aabbFirstPos, pos);
	const glm::ivec3 maxs = glm::max(_aabbFirstPos, pos) + (size - 1);
	voxel::Region modifiedRegion;
	glm::ivec3 minsMirror = mins;
	glm::ivec3 maxsMirror = maxs;
	if (!getMirrorAABB(minsMirror, maxsMirror)) {
		if (voxedit::tool::aabb(wrapper, mins, maxs, _cursorVoxel, _modifierType, &modifiedRegion)) {
			modified(modifiedRegion);
		}
		return true;
	}
	const math::AABB<int> first(mins, maxs);
	const math::AABB<int> second(minsMirror, maxsMirror);
	voxel::Region modifiedRegionMirror;
	if (math::intersects(first, second)) {
		if (voxedit::tool::aabb(wrapper, mins, maxsMirror, _cursorVoxel, _modifierType, &modifiedRegionMirror)) {
			modified(modifiedRegionMirror);
		}
	} else {
		if (voxedit::tool::aabb(wrapper, mins, maxs, _cursorVoxel, _modifierType, &modifiedRegion)) {
			modified(modifiedRegion);
		}
		if (voxedit::tool::aabb(wrapper, minsMirror, maxsMirror, _cursorVoxel, _modifierType, &modifiedRegionMirror)) {
			modified(modifiedRegionMirror);
		}
	}
	return true;
}

void SceneManager::undo() {
	voxel::RawVolume* v = _mementoHandler.undo();
	if (v == nullptr) {
		return;
	}
	setNewVolume(v);
	modified(v->region(), false);
}

void SceneManager::redo() {
	voxel::RawVolume* v = _mementoHandler.redo();
	if (v == nullptr) {
		return;
	}
	setNewVolume(v);
	modified(v->region(), false);
}

void SceneManager::resetLastTrace() {
	_lastRaytraceX = _lastRaytraceY = -1;
}

void SceneManager::setNewVolume(voxel::RawVolume* volume) {
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

bool SceneManager::newVolume(bool force) {
	if (dirty() && !force) {
		return false;
	}
	const voxel::Region region(glm::ivec3(0), glm::ivec3(size() - 1));
	_mementoHandler.clearStates();
	setNewVolume(new voxel::RawVolume(region));
	modified(region);
	_dirty = false;
	return true;
}

void SceneManager::rotate(int angleX, int angleY, int angleZ) {
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = voxel::rotateVolume(model, glm::vec3(angleX, angleY, angleZ), voxel::Voxel(), false);
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void SceneManager::move(int x, int y, int z) {
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = new voxel::RawVolume(model->region());
	voxel::RawVolumeMoveWrapper wrapper(newVolume);
	voxel::moveVolume(&wrapper, model, glm::ivec3(x, y, z));
	setNewVolume(newVolume);
	modified(newVolume->region());
}

bool SceneManager::setGridResolution(int resolution) {
	const bool ret = gridRenderer().setGridResolution(resolution);
	if (!ret) {
		return false;
	}

	const int res = gridResolution();
	if (_aabbFirstPos.x % res != 0) {
		_aabbFirstPos.x = (_aabbFirstPos.x / res) * res;
	}
	if (_aabbFirstPos.y % res != 0) {
		_aabbFirstPos.y = (_aabbFirstPos.y / res) * res;
	}
	if (_aabbFirstPos.z % res != 0) {
		_aabbFirstPos.z = (_aabbFirstPos.z / res) * res;
	}

	setCursorPosition(_cursorPos, true);

	return true;
}

void SceneManager::render(const video::Camera& camera) {
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
		const float delta = 0.001f;
		const float size = gridRenderer().gridResolution() + delta;
		if (getMirrorAABB(minsMirror, maxsMirror)) {
			const math::AABB<int> first(mins, maxs);
			const math::AABB<int> second(minsMirror, maxsMirror);
			if (math::intersects(first, second)) {
				_shapeBuilder.cube(glm::vec3(mins) - delta, glm::vec3(maxsMirror) + size);
			} else {
				_shapeBuilder.cube(glm::vec3(mins) - delta, glm::vec3(maxs) + size);
				_shapeBuilder.cube(glm::vec3(minsMirror) - delta, glm::vec3(maxsMirror) + size);
			}
		} else {
			_shapeBuilder.cube(glm::vec3(mins) - delta, glm::vec3(maxs) + size);
		}
		_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
		_shapeRenderer.render(_aabbMeshIndex, camera);
	}

	const glm::mat4& translate = glm::translate(glm::vec3(cursorPosition()));
	const glm::mat4& scale = glm::scale(translate, glm::vec3(gridRenderer().gridResolution()));
	_shapeRenderer.render(_voxelCursorMesh, camera, scale);
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

void SceneManager::construct() {
	for (size_t i = 0; i < lengthof(DIRECTIONS); ++i) {
		core::Command::registerActionButton(
				core::string::format("movecursor%s", DIRECTIONS[i].postfix),
				_move[i]);
	}

	core::Command::registerCommand("crop",
			[&] (const core::CmdArgs& args) {crop();}).setHelp(
			"Crop the volume");

	core::Command::registerCommand("scalehalf",
			[&] (const core::CmdArgs& args) {scaleHalf();}).setHelp(
			"Scale your volume by 50%");

	core::Command::registerCommand("setvoxelresolution",
			[&] (const core::CmdArgs& args) {
				const int argc = args.size();
				if (argc == 1) {
					const int size = core::string::toInt(args[0]);
					setGridResolution(size);
				} else {
					Log::warn("Expected to get a voxel resolution >= 1");
				}
			}).setHelp("");
	core::Command::registerCommand("setreferenceposition", [&] (const core::CmdArgs& args) {
		if (args.size() != 3) {
			Log::info("Expected to get x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		setReferencePosition(glm::ivec3(x, y, z));
	}).setHelp("Set the reference position to the specified position");

	core::Command::registerCommand("movecursor", [this] (const core::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Expected to get relative x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		moveCursor(x, y, z);
	}).setHelp("Move the cursor by the specified offsets");

	core::Command::registerCommand("cursor", [this] (const core::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Expected to get x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		setCursorPosition(glm::ivec3(x, y, z), true);
	}).setHelp("Set the cursor to the specified position");
	core::Command::registerCommand("setreferencepositiontocursor",
			[&] (const core::CmdArgs& args) {setReferencePosition(cursorPosition());}).setHelp(
			"Set the reference position to the current cursor position");
	core::Command::registerCommand("rotatex",
			[&] (const core::CmdArgs& args) {rotate(90, 0, 0);}).setHelp(
			"Rotate the volume around the x axis");
	core::Command::registerCommand("rotatey",
			[&] (const core::CmdArgs& args) {rotate(0, 90, 0);}).setHelp(
			"Rotate the volume around the y axis");
	core::Command::registerCommand("rotatez",
			[&] (const core::CmdArgs& args) {rotate(0, 0, 90);}).setHelp(
			"Rotate the volume around the z axis");
	core::Command::registerCommand("resize", [this] (const core::CmdArgs& args) {
		const int argc = args.size();
		if (argc == 1) {
			const int size = core::string::toInt(args[0]);
			extend(glm::ivec3(size));
		} else if (argc == 3) {
			glm::ivec3 size;
			for (int i = 0; i < argc; ++i) {
				size[i] = core::string::toInt(args[i]);
			}
			extend(size);
		} else {
			extend(glm::ivec3(1));
		}
	}).setHelp("Resize your volume about given x, y and z size");
	core::Command::registerCommand("undo",
			[&] (const core::CmdArgs& args) {undo();}).setHelp(
			"Undo your last step");
	core::Command::registerCommand("redo",
			[&] (const core::CmdArgs& args) {undo();}).setHelp(
			"Redo your last step");
	core::Command::registerCommand("rotate", [&] (const core::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Expected to get x, y and z angles in degrees");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		rotate(x, y, z);
	}).setHelp("Rotate voxels by the given angles (in degree)");
}

bool SceneManager::init() {
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

	_autoSaveSecondsDelay = core::Var::get(cfg::VoxEditAutoSaveSeconds, "180");
	const core::TimeProviderPtr& timeProvider = core::App::getInstance()->timeProvider();
	_lastAutoSave = timeProvider->tickSeconds();

	_mirrorMeshIndex = -1;
	_aabbMeshIndex = -1;
	for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
		_planeMeshIndex[i] = -1;
	}

	_lockedAxis = math::Axis::None;
	_mirrorAxis = math::Axis::None;
	return true;
}

void SceneManager::update(uint64_t time) {
	for (size_t i = 0; i < lengthof(DIRECTIONS); ++i) {
		if (!_move[i].pressed()) {
			continue;
		}
		if (time - _lastMove[i] < 125ul) {
			continue;
		}
		const Direction& dir = DIRECTIONS[i];
		moveCursor(dir.x, dir.y, dir.z);
		_lastMove[i] = time;
	}
	autosave();
	extractVolume();
}

void SceneManager::shutdown() {
	--_initialized;
	if (_initialized != 0) {
		return;
	}
	std::vector<voxel::RawVolume*> old = _volumeRenderer.shutdown();
	for (voxel::RawVolume* v : old) {
		delete v;
	}

	_axis.shutdown();
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_gridRenderer.shutdown();
	_mementoHandler.clearStates();
}

bool SceneManager::extractVolume() {
	if (_extract) {
		const size_t n = _extractRegions.size();
		Log::debug("Extract the meshes for %i regions", (int)n);
		if (n > 0) {
			// extract n regions max per frame
			const size_t MaxPerFrame = 4;
			const size_t x = std::min(MaxPerFrame, n);
			for (size_t i = 0; i < x; ++i) {
				if (!_volumeRenderer.extract(ModelVolumeIndex, _extractRegions[i])) {
					Log::error("Failed to extract the model mesh");
				}
			}
			// delete the first n entries and compact the memory of the buffer
			RegionQueue(_extractRegions.begin() + x, _extractRegions.end()).swap(_extractRegions);
		}
		_extract = !_extractRegions.empty();
		return true;
	}
	return false;
}

void SceneManager::noise(int octaves, float lacunarity, float frequency, float gain, voxel::noisegen::NoiseType type) {
	math::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::noisegen::generate(wrapper, octaves, lacunarity, frequency, gain, type, random);
	modified(wrapper.dirtyRegion());
}

void SceneManager::createCactus() {
	math::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::cactus::createCactus(wrapper, _referencePos, 18, 2, random);
	modified(wrapper.dirtyRegion());
}

void SceneManager::createCloud() {
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
		modified(wrapper.dirtyRegion());
	}
}

void SceneManager::createPlant(voxel::PlantType type) {
	voxel::PlantGenerator g;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	if (type == voxel::PlantType::Flower) {
		g.createFlower(5, _referencePos, wrapper);
	} else if (type == voxel::PlantType::Grass) {
		g.createGrass(10, _referencePos, wrapper);
	} else if (type == voxel::PlantType::Mushroom) {
		g.createMushroom(7, _referencePos, wrapper);
	}
	g.shutdown();
	modified(wrapper.dirtyRegion());
}

void SceneManager::createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx) {
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::building::createBuilding(wrapper, _referencePos, type);
	modified(wrapper.dirtyRegion());
}

void SceneManager::createTree(voxel::TreeContext ctx) {
	math::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	ctx.pos = _referencePos;
	voxel::tree::createTree(wrapper, ctx, random);
	modified(wrapper.dirtyRegion());
}

void SceneManager::setCursorVoxel(const voxel::Voxel& voxel) {
	_cursorVoxel = voxel;
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(voxel::getMaterialColor(voxel), 0.7f));
	_shapeBuilder.setPosition(glm::zero<glm::vec3>());
	_shapeBuilder.cube(glm::vec3(-0.01f), glm::vec3(1.01f));
	_shapeRenderer.createOrUpdate(_voxelCursorMesh, _shapeBuilder);
}

void SceneManager::setReferencePosition(const glm::ivec3& pos) {
	_shapeBuilder.clear();
	_shapeBuilder.setColor(core::Color::alpha(core::Color::SteelBlue, 0.8f));
	const glm::vec3 posalgined{pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f};
	_shapeBuilder.setPosition(posalgined);
	_shapeBuilder.sphere(8, 6, 0.5f);
	_shapeRenderer.createOrUpdate(_referencePointMesh, _shapeBuilder);
	_referencePos = pos;
}

void SceneManager::moveCursor(int x, int y, int z) {
	glm::ivec3 p = cursorPosition();
	p.x += x;
	p.y += y;
	p.z += z;
	setCursorPosition(p, true);
}

void SceneManager::setCursorPosition(glm::ivec3 pos, bool force) {
	const voxel::RawVolume* v = modelVolume();
	if (v == nullptr) {
		return;
	}

	const int res = gridRenderer().gridResolution();
	if (pos.x % res != 0) {
		pos.x = (pos.x / res) * res;
	}
	if (pos.y % res != 0) {
		pos.y = (pos.y / res) * res;
	}
	if (pos.z % res != 0) {
		pos.z = (pos.z / res) * res;
	}
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

	const voxel::Region& region = v->region();
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

bool SceneManager::renderAxis() const {
	return _renderAxis;
}

void SceneManager::setRenderAxis(bool renderAxis) {
	_renderAxis = renderAxis;
}

bool SceneManager::renderLockAxis() const {
	return _renderLockAxis;
}

void SceneManager::setRenderLockAxis(bool renderLockAxis) {
	_renderLockAxis = renderLockAxis;
}

bool SceneManager::renderShadow() const {
	return _renderShadow;
}

void SceneManager::setRenderShadow(bool shadow) {
	_renderShadow = shadow;
	Log::debug("render shadow: %i", shadow ? 1 : 0);
}

bool SceneManager::addModifierType(ModifierType type) {
	if ((_modifierType & type) == type) {
		return false;
	}
	_modifierType &= type;
	// the modifier type has an influence on which voxel is taken. So make
	// sure the next trace is executed even if we don't move the mouse.
	resetLastTrace();
	return true;
}

void SceneManager::setModifierType(ModifierType type) {
	_modifierType = type;
	// the modifier type has an influence on which voxel is taken. So make
	// sure the next trace is executed even if we don't move the mouse.
	resetLastTrace();
}

ModifierType SceneManager::modifierType() const {
	return _modifierType;
}

bool SceneManager::modifierTypeRequiresExistingVoxel() const {
	return (_modifierType & ModifierType::Delete) == ModifierType::Delete || (_modifierType & ModifierType::Update) == ModifierType::Delete;
}

bool SceneManager::trace(const video::Camera& camera, bool force) {
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

int SceneManager::getIndexForAxis(math::Axis axis) const {
	if (axis == math::Axis::X) {
		return 0;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 2;
}

int SceneManager::getIndexForMirrorAxis(math::Axis axis) const {
	if (axis == math::Axis::X) {
		return 2;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 0;
}

void SceneManager::updateShapeBuilderForPlane(bool mirror, const glm::ivec3& pos, math::Axis axis, const glm::vec4& color) {
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

void SceneManager::updateLockedPlane(math::Axis axis) {
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

math::Axis SceneManager::mirrorAxis() const {
	return _mirrorAxis;
}

void SceneManager::setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos) {
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

void SceneManager::updateMirrorPlane() {
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

void SceneManager::setLockedAxis(math::Axis axis, bool unlock) {
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
