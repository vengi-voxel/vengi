/**
 * @file
 */

#include "SceneManager.h"

#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/VolumeCropper.h"
#include "voxel/polyvox/VolumeRotator.h"
#include "voxel/polyvox/VolumeMover.h"
#include "voxel/polyvox/VolumeRescaler.h"
#include "voxel/polyvox/VolumeVisitor.h"
#include "voxel/polyvox//RawVolumeWrapper.h"
#include "voxel/polyvox//RawVolumeMoveWrapper.h"
#include "voxel/polyvox//Mesh.h"
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
#include "voxelformat/VXMFormat.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedBlendMode.h"
#include "video/Ray.h"
#include "math/Random.h"
#include "core/command/Command.h"
#include "core/Array.h"
#include "core/App.h"
#include "core/Log.h"
#include "core/Color.h"
#include "core/String.h"
#include "core/GLM.h"
#include "io/Filesystem.h"

#include "AxisUtil.h"
#include "Config.h"
#include "tool/Crop.h"
#include "tool/Resize.h"
#include "ImportHeightmap.h"

#include <set>

#define VOXELIZER_IMPLEMENTATION
#include "voxelizer.h"

namespace voxedit {

const int leafSize = 8;

SceneManager::SceneManager() :
		_gridRenderer() {
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
	_volumeRenderer.toMesh(&mesh);
	return voxel::exportMesh(&mesh, filePtr->name().c_str());
}

voxel::Region SceneManager::region() const {
	return _volumeRenderer.region();
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
	const int layerId = _layerMgr.activeLayer();
	voxel::RawVolume* v = volume(layerId);
	if (v == nullptr) {
		return false;
	}
	const image::ImagePtr& img = image::loadImage(file, false);
	if (!img->isLoaded()) {
		return false;
	}
	voxel::RawVolumeWrapper wrapper(v);
	voxedit::importHeightmap(wrapper, img);
	modified(layerId, wrapper.dirtyRegion());
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
		autoSaveFilename = "autosave-noname.vox";
	} else {
		if (core::string::startsWith(_lastFilename.c_str(), "autosave-")) {
			autoSaveFilename = _lastFilename;
		} else {
			autoSaveFilename = "autosave-" + _lastFilename;
		}
	}
	if (save(autoSaveFilename, true)) {
		Log::info("Autosave file %s", autoSaveFilename.c_str());
	} else {
		Log::warn("Failed to autosave");
	}
	_lastAutoSave = timeProvider->tickSeconds();
}

bool SceneManager::save(const std::string& file, bool autosave) {
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
	voxel::VoxelVolumes volumes;
	const int layers = (int)_layerMgr.layers().size();
	for (int idx = 0; idx < layers; ++idx) {
		voxel::RawVolume* v = _volumeRenderer.volume(idx);
		if (v == nullptr) {
			continue;
		}
		if (_volumeRenderer.empty(idx)) {
			continue;
		}
		const Layer& layer = _layerMgr.layer(idx);
		volumes.push_back(voxel::VoxelVolume(v, layer.name, layer.visible));
	}

	if (volumes.empty()) {
		return false;
	}

	if (ext == "qbt") {
		voxel::QBTFormat f;
		saved = f.saveGroups(volumes, filePtr);
	} else if (ext == "vox") {
		voxel::VoxFormat f;
		saved = f.saveGroups(volumes, filePtr);
	} else if (ext == "qb") {
		voxel::QBFormat f;
		saved = f.saveGroups(volumes, filePtr);
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
	const int layerId = _layerMgr.activeLayer();
	voxel::RawVolumeWrapper wrapper(volume(layerId));
	voxel::moveVolume(&wrapper, newVolume, _referencePos);
	modified(layerId, wrapper.dirtyRegion());
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
	voxel::VoxelVolumes newVolumes;

	const std::string& ext = filePtr->extension();
	_lastFilename = filePtr->fileName() + "." + ext;
	if (ext == "qbt") {
		voxel::QBTFormat f;
		newVolumes = f.loadGroups(filePtr);
	} else if (ext == "vox") {
		voxel::VoxFormat f;
		newVolumes = f.loadGroups(filePtr);
	} else if (ext == "qb") {
		voxel::QBFormat f;
		newVolumes = f.loadGroups(filePtr);
	} else if (ext == "vxm") {
		voxel::VXMFormat f;
		newVolumes = f.loadGroups(filePtr);
	} else {
		Log::error("Failed to load model file %s - unsupported file format", file.c_str());
		return false;
	}
	if (newVolumes.empty()) {
		Log::error("Failed to load model file %s", file.c_str());
		return false;
	}
	Log::info("Load model file %s with %i layers", file.c_str(), (int)newVolumes.size());
	if (!setNewVolumes(newVolumes)) {
		return false;
	}
	_needAutoSave = false;
	_extract = true;
	_dirty = false;
	return true;
}

void SceneManager::setMousePos(int x, int y) {
	_mouseX = x;
	_mouseY = y;
}

void SceneManager::modified(int layerId, const voxel::Region& modifiedRegion, bool markUndo) {
	if (!modifiedRegion.isValid()) {
		return;
	}
	if (markUndo) {
		_mementoHandler.markUndo(layerId, _volumeRenderer.volume(layerId));
	}
	_extractRegions.push_back({modifiedRegion, layerId});
	_dirty = true;
	_needAutoSave = true;
	_extract = true;
}

void SceneManager::crop() {
	const int layerId = _layerMgr.activeLayer();
	if (_volumeRenderer.empty(layerId)) {
		Log::info("Empty volumes can't be cropped");
		return;
	}
	voxel::RawVolume* newVolume = voxedit::tool::crop(volume(layerId));
	if (newVolume == nullptr) {
		return;
	}
	setNewVolume(layerId, newVolume);
	modified(layerId, newVolume->region());
}

void SceneManager::resize(const glm::ivec3& size) {
	const int layerId = _layerMgr.activeLayer();
	voxel::RawVolume* newVolume = voxedit::tool::resize(volume(layerId), size);
	if (newVolume == nullptr) {
		return;
	}
	setNewVolume(layerId, newVolume);
	modified(layerId, newVolume->region());
}

void SceneManager::pointCloud(const glm::vec3* vertices, const glm::vec3 *vertexColors, size_t amount) {
	glm::ivec3 mins(std::numeric_limits<glm::ivec3::value_type>::max());
	glm::ivec3 maxs(std::numeric_limits<glm::ivec3::value_type>::min());

	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	materialColors.erase(materialColors.begin());
	const int layerId = _layerMgr.activeLayer();
	voxel::RawVolumeWrapper wrapper(volume(layerId));

	const glm::ivec3& cursorPos = cursorPosition();
	bool change = false;
	for (size_t idx = 0u; idx < amount; ++idx) {
		const glm::vec3& vertex = vertices[idx];
		const glm::vec3& color = vertexColors[idx];
		const glm::ivec3 pos(cursorPos.x + vertex.x, cursorPos.y + vertex.y, cursorPos.z + vertex.z);
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
	modified(layerId, modifiedRegion);
}

voxel::RawVolume* SceneManager::volume(int idx) {
	voxel::RawVolume* v = _volumeRenderer.volume(idx);
	//core_assert_msg(v != nullptr, "Volume for index %i is null", idx);
	return v;
}

voxel::RawVolume* SceneManager::modelVolume() {
	const int idx = _layerMgr.activeLayer();
	return volume(idx);
}

void SceneManager::undo() {
	const LayerState& s = _mementoHandler.undo();
	voxel::RawVolume* v = s.volume;
	if (v == nullptr) {
		return;
	}
	setNewVolume(s.layer, v);
	_layerMgr.setActiveLayer(s.layer);
	modified(s.layer, v->region(), false);
}

void SceneManager::redo() {
	const LayerState& s = _mementoHandler.redo();
	voxel::RawVolume* v = s.volume;
	if (v == nullptr) {
		return;
	}
	setNewVolume(s.layer, v);
	_layerMgr.setActiveLayer(s.layer);
	modified(s.layer, v->region(), false);
}

void SceneManager::resetLastTrace() {
	_lastRaytraceX = _lastRaytraceY = -1;
}

bool SceneManager::setNewVolumes(const voxel::VoxelVolumes& volumes) {
	const int size = (int)volumes.size();
	if (size == 0) {
		return newScene(true);
	}
	const int maxLayers = _layerMgr.maxLayers();
	if (size > maxLayers) {
		Log::error("Max supported layer size exceeded: %i (max supported: %i)",
				size, maxLayers);
		return false;
	}
	for (int idx = 0; idx < maxLayers; ++idx) {
		_layerMgr.deleteLayer(idx, true);
	}
	for (int idx = 0; idx < size; ++idx) {
		const int layerId = _layerMgr.addLayer(volumes[idx].name.c_str(), volumes[idx].visible, volumes[idx].volume);
		if (layerId < 0) {
			return newScene(true);
		}
	}
	_mementoHandler.clearStates();
	_layerMgr.findNewActiveLayer();
	const int layerId = _layerMgr.activeLayer();
	// push the initial state of the current layer to the memento handler to
	// be able to undo your next step
	_mementoHandler.markUndo(layerId, _volumeRenderer.volume(layerId));
	_dirty = false;
	_result = voxel::PickResult();
	const glm::ivec3 pos = _cursorPos;
	_cursorPos = pos * 10 + 10;
	setCursorPosition(pos);
	resetLastTrace();
	return true;
}

bool SceneManager::setNewVolume(int idx, voxel::RawVolume* volume) {
	if (idx < 0 || idx >= _layerMgr.maxLayers()) {
		return false;
	}
	const voxel::Region& region = volume->region();
	delete _volumeRenderer.setVolume(idx, volume);
	_layerMgr.layer(idx).valid = volume != nullptr;

	if (volume != nullptr) {
		_gridRenderer.update(region);
	} else {
		_gridRenderer.clear();
	}

	_dirty = false;
	_result = voxel::PickResult();
	_extractRegions.push_back({region, idx});
	const glm::ivec3 pos = _cursorPos;
	_cursorPos = pos * 10 + 10;
	setCursorPosition(pos);
	setReferencePosition(region.getCentre());
	resetLastTrace();
	return true;
}

// TODO: dialog that selects the palette and mins/maxs of the volume
bool SceneManager::newScene(bool force) {
	if (dirty() && !force) {
		return false;
	}
	const int layers = _layerMgr.maxLayers();
	for (int idx = 0; idx < layers; ++idx) {
		_layerMgr.deleteLayer(idx, true);
	}
	core_assert_always(_layerMgr.validLayers() == 0);
	const voxel::Region region(glm::ivec3(0), glm::ivec3(size() - 1));
	setReferencePosition(region.getCentre());
	_mementoHandler.clearStates();
	core_assert_always(_layerMgr.addLayer("", true, new voxel::RawVolume(region)) != -1);
	_layerMgr.setActiveLayer(0);
	modified(_layerMgr.activeLayer(), region);
	_dirty = false;
	core_assert_always(_layerMgr.validLayers() == 1);
	return true;
}

void SceneManager::rotate(int angleX, int angleY, int angleZ) {
	const int layerId = _layerMgr.activeLayer();
	const voxel::RawVolume* model = volume(layerId);
	voxel::RawVolume* newVolume = voxel::rotateVolume(model, glm::vec3(angleX, angleY, angleZ), voxel::Voxel(), false);
	setNewVolume(layerId, newVolume);
	modified(layerId, newVolume->region());
}

void SceneManager::move(int x, int y, int z) {
	const int layerId = _layerMgr.activeLayer();
	const voxel::RawVolume* model = volume(layerId);
	voxel::RawVolume* newVolume = new voxel::RawVolume(model->region());
	voxel::RawVolumeMoveWrapper wrapper(newVolume);
	voxel::moveVolume(&wrapper, model, glm::ivec3(x, y, z));
	setNewVolume(layerId, newVolume);
	modified(layerId, newVolume->region());
}

bool SceneManager::setGridResolution(int resolution) {
	const bool ret = gridRenderer().setGridResolution(resolution);
	if (!ret) {
		return false;
	}

	const int res = gridResolution();
	_modifier.setGridResolution(res);
	setCursorPosition(_cursorPos, true);

	return true;
}

void SceneManager::render(const video::Camera& camera) {
	const bool depthTest = video::enable(video::State::DepthTest);
	_gridRenderer.render(camera, modelVolume()->region());
	_volumeRenderer.render(camera, _renderShadow);
	_modifier.render(camera);

	// TODO: render error if rendered last - but be before grid renderer to get transparency.
	if (_renderLockAxis) {
		for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
			_shapeRenderer.render(_planeMeshIndex[i], camera);
		}
	}
	if (renderAxis()) {
		_axis.render(camera);
	}
	if (!depthTest) {
		video::disable(video::State::DepthTest);
	}
	_shapeRenderer.render(_referencePointMesh, camera);
}

void SceneManager::construct() {
	_layerMgr.construct();
	_modifier.construct();

	for (size_t i = 0; i < lengthof(DIRECTIONS); ++i) {
		core::Command::registerActionButton(
				core::string::format("movecursor%s", DIRECTIONS[i].postfix),
				_move[i]);
	}

	core::Command::registerCommand("noise", [&] (const core::CmdArgs& args) {
		const int argc = args.size();
		if (argc != 4) {
			Log::info("Usage: noise <octaves> <lacunarity> <frequency> <gain>");
			return;
		}
		int octaves = core::string::toInt(args[0]);
		float lacunarity = core::string::toFloat(args[0]);
		float frequency = core::string::toFloat(args[0]);
		float gain = core::string::toFloat(args[0]);
		voxel::noisegen::NoiseType type = voxel::noisegen::NoiseType::ridgedMF;
		noise(octaves, lacunarity, frequency, gain, type);
	}).setHelp("Fill the volume with noise");

	core::Command::registerCommand("crop",
			[&] (const core::CmdArgs& args) {crop();}).setHelp(
			"Crop the volume");

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
			resize(glm::ivec3(size));
		} else if (argc == 3) {
			glm::ivec3 size;
			for (int i = 0; i < argc; ++i) {
				size[i] = core::string::toInt(args[i]);
			}
			resize(size);
		} else {
			resize(glm::ivec3(1));
		}
	}).setHelp("Resize your volume about given x, y and z size");
	core::Command::registerCommand("undo",
			[&] (const core::CmdArgs& args) {undo();}).setHelp(
			"Undo your last step");
	core::Command::registerCommand("redo",
			[&] (const core::CmdArgs& args) {redo();}).setHelp(
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
	}).setHelp("Rotate scene by the given angles (in degree)");
	core::Command::registerCommand("rotatex", [&] (const core::CmdArgs& args) {
		const int deg = args.size() == 1 ? core::string::toInt(args[0]) : 90;
		rotate(deg, 0, 0);
	}).setHelp("Rotate scene by the given angles (in degree)");
	core::Command::registerCommand("rotatey", [&] (const core::CmdArgs& args) {
		const int deg = args.size() == 1 ? core::string::toInt(args[0]) : 90;
		rotate(0, deg, 0);
	}).setHelp("Rotate scene by the given angles (in degree)");
	core::Command::registerCommand("rotatez", [&] (const core::CmdArgs& args) {
		const int deg = args.size() == 1 ? core::string::toInt(args[0]) : 90;
		rotate(0, 0, deg);
	}).setHelp("Rotate scene by the given angles (in degree)");

	core::Command::registerCommand("layerdetails", [&] (const core::CmdArgs& args) {
		for (int idx = 0; idx < (int)_layerMgr.layers().size(); ++idx) {
			const Layer& layer = _layerMgr.layer(idx);
			if (!layer.valid) {
				continue;
			}
			Log::info("Layer %i:", idx);
			Log::info(" - name:    %s", layer.name.c_str());
			Log::info(" - visible: %s", layer.visible ? "true" : "false");
			const voxel::RawVolume* volume = _volumeRenderer.volume(idx);
			core_assert_always(volume != nullptr);
			const voxel::Region& region = volume->region();
			Log::info(" - region:");
			Log::info("   - mins:   %i:%i:%i", region.getLowerX(), region.getLowerY(), region.getLowerZ());
			Log::info("   - maxs:   %i:%i:%i", region.getUpperX(), region.getUpperY(), region.getUpperZ());
			Log::info("   - cells:  %i:%i:%i", region.getWidthInCells(), region.getHeightInCells(), region.getDepthInCells());
			Log::info("   - voxels: %i:%i:%i", region.getWidthInVoxels(), region.getHeightInVoxels(), region.getDepthInVoxels());
		}
	}).setHelp("Show details to all layers");

	core::Command::registerCommand("animate", [&] (const core::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: animate <framedelay>");
			Log::info("framedelay of 0 will stop the animation");
			return;
		}
		_animationSpeed = core::string::toInt(args[0]);
	}).setHelp("Animate all visible layers with the given delay in millis between the frames");
	core::Command::registerCommand("pickcolor", [&] (const core::CmdArgs& args) {
		if (!voxel::isAir(_hitCursorVoxel.getMaterial())) {
			_modifier.setCursorVoxel(_hitCursorVoxel);
		}
	}).setHelp("Pick the current selected color");
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
	_layerMgr.init();
	_layerMgr.registerListener(this);
	_modifier.init();

	_autoSaveSecondsDelay = core::Var::get(cfg::VoxEditAutoSaveSeconds, "180");
	const core::TimeProviderPtr& timeProvider = core::App::getInstance()->timeProvider();
	_lastAutoSave = timeProvider->tickSeconds();

	for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
		_planeMeshIndex[i] = -1;
	}

	_lockedAxis = math::Axis::None;
	return true;
}

void SceneManager::animate(uint64_t time) {
	if (_animationSpeed <= 0) {
		return;
	}
	if (_nextFrameSwitch <= time) {
		_nextFrameSwitch = time + _animationSpeed;
		const int layers = (int)_layerMgr.layers().size();
		const int roundTrip = layers + _currentAnimationLayer;
		for (int idx = _currentAnimationLayer + 1; idx < roundTrip; ++idx) {
			const Layer& layer = _layerMgr.layer(idx % layers);
			if (layer.valid && layer.visible) {
				 _layerMgr.hideLayer(_currentAnimationLayer, true);
				_currentAnimationLayer = idx % layers;
				_layerMgr.hideLayer(_currentAnimationLayer, false);
				return;
			}
		}
	}
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
	animate(time);
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

	_modifier.shutdown();
	_layerMgr.unregisterListener(this);
	_layerMgr.shutdown();
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
			int lastLayer = _layerMgr.activeLayer();
			for (size_t i = 0; i < x; ++i) {
				const bool updateBuffers = i == x - 1 || lastLayer != _extractRegions[i].layer;
				if (!_volumeRenderer.extract(_extractRegions[i].layer, _extractRegions[i].region, updateBuffers)) {
					Log::error("Failed to extract the model mesh");
				}
				lastLayer = _extractRegions[i].layer;
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
	const int layerId = _layerMgr.activeLayer();
	voxel::RawVolumeWrapper wrapper(volume(layerId));
	voxel::noisegen::generate(wrapper, octaves, lacunarity, frequency, gain, type, random);
	modified(layerId, wrapper.dirtyRegion());
}

void SceneManager::createCactus() {
	math::Random random;
	const int layerId = _layerMgr.activeLayer();
	voxel::RawVolumeWrapper wrapper(volume(layerId));
	voxel::cactus::createCactus(wrapper, _referencePos, 18, 2, random);
	modified(layerId, wrapper.dirtyRegion());
}

void SceneManager::createCloud() {
	const int layerId = _layerMgr.activeLayer();
	voxel::RawVolumeWrapper wrapper(volume(layerId));
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
		modified(layerId, wrapper.dirtyRegion());
	}
}

void SceneManager::createPlant(voxel::PlantType type) {
	voxel::PlantGenerator g;
	const int layerId = _layerMgr.activeLayer();
	voxel::RawVolumeWrapper wrapper(volume(layerId));
	if (type == voxel::PlantType::Flower) {
		g.createFlower(5, _referencePos, wrapper);
	} else if (type == voxel::PlantType::Grass) {
		g.createGrass(10, _referencePos, wrapper);
	} else if (type == voxel::PlantType::Mushroom) {
		g.createMushroom(7, _referencePos, wrapper);
	}
	g.shutdown();
	modified(layerId, wrapper.dirtyRegion());
}

void SceneManager::createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx) {
	const int layerId = _layerMgr.activeLayer();
	voxel::RawVolumeWrapper wrapper(volume(layerId));
	voxel::building::createBuilding(wrapper, _referencePos, type);
	modified(layerId, wrapper.dirtyRegion());
}

void SceneManager::createTree(voxel::TreeContext ctx) {
	math::Random random;
	const int layerId = _layerMgr.activeLayer();
	voxel::RawVolumeWrapper wrapper(volume(layerId));
	ctx.pos = _referencePos;
	voxel::tree::createTree(wrapper, ctx, random);
	modified(layerId, wrapper.dirtyRegion());
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
	const int res = gridRenderer().gridResolution();
	p.x += x * res;
	p.y += y * res;
	p.z += z * res;
	setCursorPosition(p, true);
}

void SceneManager::setCursorPosition(glm::ivec3 pos, bool force) {
	const voxel::RawVolume* v = modelVolume();
	if (v == nullptr) {
		return;
	}

	const int res = gridRenderer().gridResolution();
	const voxel::Region& region = v->region();
	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3 delta = pos - mins;
	if (delta.x % res != 0) {
		pos.x = mins.x + (delta.x / res) * res;
	}
	if (delta.y % res != 0) {
		pos.y = mins.y + (delta.y / res) * res;
	}
	if (delta.z % res != 0) {
		pos.z = mins.z + (delta.z / res) * res;
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
}

bool SceneManager::trace(const video::Camera& camera, bool force) {
	const voxel::RawVolume* model = modelVolume();
	if (model == nullptr) {
		return false;
	}

	if (_lastRaytraceX != _mouseX || _lastRaytraceY != _mouseY || force) {
		core_trace_scoped(EditorSceneOnProcessUpdateRay);
		_lastRaytraceX = _mouseX;
		_lastRaytraceY = _mouseY;

		const video::Ray& ray = camera.mouseRay(glm::ivec2(_mouseX, _mouseY));
		const glm::vec3& dirWithLength = ray.direction * camera.farPlane();
		static constexpr voxel::Voxel air;

		_result.didHit = false;
		_result.validPreviousPosition = false;
		raycastWithDirection(model, ray.origin, dirWithLength, [&] (voxel::RawVolume::Sampler& sampler) {
			if (sampler.voxel() != air) {
				_result.didHit = true;
				_result.hitVoxel = sampler.position();
				return false;
			}

			if (sampler.currentPositionValid()) {
				if (_lockedAxis != math::Axis::None) {
					if ((_lockedAxis & math::Axis::X) != math::Axis::None) {
						if (sampler.position()[0] == _cursorPos[0]) {
							return false;
						}
					}
					if ((_lockedAxis & math::Axis::Y) != math::Axis::None) {
						if (sampler.position()[1] == _cursorPos[1]) {
							return false;
						}
					}
					if ((_lockedAxis & math::Axis::Z) != math::Axis::None) {
						if (sampler.position()[2] == _cursorPos[2]) {
							return false;
						}
					}
				}

				_result.validPreviousPosition = true;
				_result.previousPosition = sampler.position();
			}
			return true;
		});

		if (_modifier.modifierTypeRequiresExistingVoxel()) {
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

		if (_result.didHit) {
			_hitCursorVoxel = model->voxel(_result.hitVoxel);
		}
	}

	return true;
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
	updateShapeBuilderForPlane(_shapeBuilder, _volumeRenderer.region(), false, _cursorPos, axis, core::Color::alpha(colors[index], 0.4f));
	_shapeRenderer.createOrUpdate(meshIndex, _shapeBuilder);
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

void SceneManager::onLayerHide(int layerId) {
	_volumeRenderer.hide(layerId, true);
}

void SceneManager::onLayerShow(int layerId) {
	_volumeRenderer.hide(layerId, false);
}

void SceneManager::onActiveLayerChanged(int old, int active) {
	const voxel::RawVolume* volume = _volumeRenderer.volume(active);
	core_assert_always(volume != nullptr);
	const voxel::Region& region = volume->region();
	_gridRenderer.update(region);
	if (!region.containsPoint(referencePosition())) {
		setReferencePosition(region.getCentre());
	}
	if (!region.containsPoint(cursorPosition())) {
		setCursorPosition(volume->region().getCentre());
	}
	resetLastTrace();
}

void SceneManager::onLayerAdded(int layerId, const Layer& layer, voxel::RawVolume* volume) {
	if (volume == nullptr) {
		const voxel::Region& region = _volumeRenderer.region();
		volume = new voxel::RawVolume(region);
	}
	delete _volumeRenderer.setVolume(layerId, volume);
	_volumeRenderer.hide(layerId, !layer.visible);
	_extractRegions.push_back({volume->region(), (int)layerId});
}

void SceneManager::onLayerDeleted(int layerId) {
	voxel::RawVolume* v = _volumeRenderer.setVolume(layerId, nullptr);
	if (v != nullptr) {
		_mementoHandler.markUndo(layerId, v);
		_volumeRenderer.update(layerId);
		delete v;
	}
}

}
