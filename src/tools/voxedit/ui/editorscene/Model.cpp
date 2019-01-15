/**
 * @file
 */

#include "Model.h"
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
#include "io/Filesystem.h"
#include "voxedit-util/tool/Crop.h"
#include "voxedit-util/tool/Expand.h"
#include "voxedit-util/tool/Fill.h"
#include "voxedit-util/ImportHeightmap.h"
#include "core/GLM.h"
#include <set>

namespace voxedit {

const int leafSize = 8;

Model::Model() :
		_gridRenderer(true, true) {
}

Model::~Model() {
	shutdown();
}

bool Model::importHeightmap(const std::string& file) {
	voxel::RawVolume* v = modelVolume();
	if (v == nullptr) {
		return false;
	}
	const image::ImagePtr& img = image::loadImage(file, false);
	if (!img->isLoaded()) {
		return false;
	}
	voxedit::importHeightmap(*v, img);
	modified(v->region());
	return true;
}

bool Model::save(const std::string& file) {
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

bool Model::prefab(const std::string& file) {
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

bool Model::load(const std::string& file) {
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
	undoHandler().clearUndoStates();
	setNewVolume(newVolume);
	modified(newVolume->region());
	_dirty = false;
	return true;
}

void Model::select(const glm::ivec3& pos) {
	voxel::RawVolume* selectionVolume = _selectionVolumeRenderer.volume(SelectionVolumeIndex);
	_extractSelection |= _selectionHandler.select(modelVolume(), selectionVolume, pos);
}

void Model::unselectAll() {
	_selectionHandler.unselectAll();
	_selectionVolumeRenderer.volume(SelectionVolumeIndex)->clear();
	_extractSelection = true;
}

void Model::setMousePos(int x, int y) {
	_mouseX = x;
	_mouseY = y;
}

void Model::modified(const voxel::Region& modifiedRegion, bool markUndo) {
	if (!modifiedRegion.isValid()) {
		return;
	}
	if (markUndo) {
		undoHandler().markUndo(modelVolume());
	}
	// TODO: handle the region
	_dirty = true;
	markExtract();
}

void Model::crop() {
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

void Model::extend(const glm::ivec3& size) {
	voxel::RawVolume* newVolume = voxedit::tool::expand(modelVolume(), size);
	if (newVolume == nullptr) {
		return;
	}
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void Model::scaleHalf() {
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

void Model::fill(const glm::ivec3& pos) {
	const bool overwrite = evalAction() == Action::OverrideVoxel;
	voxel::Region modifiedRegion;
	if (voxedit::tool::fill(*modelVolume(), pos, _lockedAxis, _shapeHandler.cursorVoxel(), overwrite, &modifiedRegion)) {
		modified(modifiedRegion);
	}
}

void Model::fill(const glm::ivec3& mins, const glm::ivec3& maxs) {
	const bool deleteVoxels = evalAction() == Action::DeleteVoxel;
	const bool overwrite = deleteVoxels ? true : evalAction() == Action::OverrideVoxel;
	voxel::Region modifiedRegion;
	voxel::Voxel voxel = deleteVoxels ? voxel::createVoxel(voxel::VoxelType::Air, 0) : _shapeHandler.cursorVoxel();
	if (voxedit::tool::aabb(*modelVolume(), mins, maxs, voxel, overwrite, &modifiedRegion)) {
		modified(modifiedRegion);
	}
}

bool Model::place() {
	voxel::Region modifiedRegion;
	const bool extract = placeCursor(&modifiedRegion);
	if (extract) {
		modified(modifiedRegion);
	}
	return extract;
}

bool Model::remove() {
	if (_selectionHandler.selectedVoxels() > 0) {
		const voxel::RawVolume* selection = _selectionVolumeRenderer.volume(SelectionVolumeIndex);
		glm::ivec3 mins(0);
		glm::ivec3 maxs(0);

		const bool extract = voxel::visitVolume(*selection, [this, &mins, &maxs] (int x, int y, int z, const voxel::Voxel& voxel) {
			modelVolume()->setVoxel(x, y, z, voxel::Voxel());
			mins.x = glm::min(mins.x, x);
			mins.y = glm::min(mins.y, y);
			mins.z = glm::min(mins.z, z);
			maxs.x = glm::max(maxs.x, x);
			maxs.y = glm::max(maxs.y, y);
			maxs.z = glm::max(maxs.z, z);
		}) > 0;
		if (extract) {
			const voxel::Region modifiedRegion(mins, maxs);
			modified(modifiedRegion);
			return true;
		}
		return false;
	}
	const bool extract = setVoxel(_cursorPos, voxel::Voxel());
	if (extract) {
		const voxel::Region modifiedRegion(_cursorPos, _cursorPos);
		modified(modifiedRegion);
	}
	return extract;
}

void Model::pointCloud(const glm::vec3* vertices, const glm::vec3 *vertexColors, size_t amount) {
	glm::ivec3 mins(std::numeric_limits<glm::ivec3::value_type>::max());
	glm::ivec3 maxs(std::numeric_limits<glm::ivec3::value_type>::min());

	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	materialColors.erase(materialColors.begin());

	for (size_t idx = 0u; idx < amount; ++idx) {
		const glm::vec3& vertex = vertices[idx];
		const glm::vec3& color = vertexColors[idx];
		const glm::ivec3 pos(_cursorPos.x + vertex.x, _cursorPos.y + vertex.y, _cursorPos.z + vertex.z);
		const glm::vec4 cvec(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, 255.0f);
		const uint8_t index = core::Color::getClosestMatch(cvec, materialColors);
		setVoxel(pos, voxel::createVoxel(voxel::VoxelType::Generic, index));
		mins = glm::min(mins, pos);
		maxs = glm::max(maxs, pos);
	}
	const voxel::Region modifiedRegion(mins, maxs);
	modified(modifiedRegion);
}

bool Model::aabbStart() {
	if (_aabbMode) {
		return false;
	}
	_aabbFirstPos = cursorPosition();
	_aabbMode = true;
	return true;
}

bool Model::aabbEnd() {
	if (!_aabbMode) {
		return false;
	}
	_aabbMode = false;
	const glm::ivec3& pos = cursorPosition();
	const glm::ivec3 mins = glm::min(_aabbFirstPos, pos);
	const glm::ivec3 maxs = glm::max(_aabbFirstPos, pos);
	fill(mins, maxs);
	return true;
}

void Model::executeAction(uint64_t now, bool start) {
	const Action execAction = evalAction();
	if (execAction == Action::None) {
		Log::warn("Nothing to execute");
		return;
	}

	core_trace_scoped(EditorSceneExecuteAction);

	if (!start) {
		if (execAction == Action::PlaceVoxels) {
			aabbEnd();
		}
		// only handled in the end-case is the aabb placement
		return;
	} else {
		if (execAction == Action::PlaceVoxels) {
			aabbStart();
			return;
		}
	}

	if (_lastAction == execAction) {
		if (now - _lastActionExecution < _actionExecutionDelay) {
			return;
		}
	}
	_lastAction = execAction;
	_lastActionExecution = now;

	bool extract = false;
	const bool didHit = _result.didHit;
	voxel::Region modifiedRegion;
	if (didHit && execAction == Action::CopyVoxel) {
		shapeHandler().setCursorVoxel(getVoxel(_cursorPos));
	} else if (didHit && execAction == Action::SelectVoxels) {
		select(_cursorPos);
	} else if (didHit && execAction == Action::OverrideVoxel) {
		extract = placeCursor(&modifiedRegion);
	} else if (didHit && execAction == Action::DeleteVoxel) {
		extract = setVoxel(_cursorPos, voxel::Voxel());
		if (extract) {
			modifiedRegion.setLowerCorner(_cursorPos);
			modifiedRegion.setUpperCorner(_cursorPos);
		}
	} else if (_result.validPreviousPosition && execAction == Action::PlaceVoxel) {
		extract = placeCursor(&modifiedRegion);
	} else if (didHit && execAction == Action::PlaceVoxel) {
		extract = placeCursor(&modifiedRegion);
	}

	if (!extract) {
		return;
	}
	resetLastTrace();
	modified(modifiedRegion);
}

void Model::undo() {
	voxel::RawVolume* v = undoHandler().undo();
	if (v == nullptr) {
		return;
	}
	setNewVolume(v);
	modified(v->region(), false);
}

void Model::redo() {
	voxel::RawVolume* v = undoHandler().redo();
	if (v == nullptr) {
		return;
	}
	setNewVolume(v);
	modified(v->region(), false);
}

bool Model::placeCursor(voxel::Region* modifiedRegion) {
	const glm::ivec3& pos = _cursorPos;
	const voxel::RawVolume* cursorVolume = cursorPositionVolume();
	const voxel::Region& cursorRegion = cursorVolume->region();
	const glm::ivec3 mins = -cursorRegion.getCentre() + pos;
	const glm::ivec3 maxs = mins + cursorRegion.getDimensionsInCells();
	const voxel::Region destReg(mins, maxs);

	int cnt = 0;
	for (int32_t z = cursorRegion.getLowerZ(); z <= cursorRegion.getUpperZ(); ++z) {
		const int destZ = destReg.getLowerZ() + z - cursorRegion.getLowerZ();
		for (int32_t y = cursorRegion.getLowerY(); y <= cursorRegion.getUpperY(); ++y) {
			const int destY = destReg.getLowerY() + y - cursorRegion.getLowerY();
			for (int32_t x = cursorRegion.getLowerX(); x <= cursorRegion.getUpperX(); ++x) {
				const voxel::Voxel& voxel = cursorVolume->voxel(x, y, z);
				if (isAir(voxel.getMaterial())) {
					continue;
				}
				const int destX = destReg.getLowerX() + x - cursorRegion.getLowerX();
				if (setVoxel(glm::ivec3(destX, destY, destZ), voxel)) {
					++cnt;
				}
			}
		}
	}

	if (cnt <= 0) {
		return false;
	}
	if (modifiedRegion != nullptr) {
		*modifiedRegion = destReg;
	}
	return true;
}

void Model::resetLastTrace() {
	_lastRaytraceX = _lastRaytraceY = -1;
}

void Model::setNewVolume(voxel::RawVolume* volume) {
	const voxel::Region& region = volume->region();

	delete _selectionVolumeRenderer.setVolume(SelectionVolumeIndex, new voxel::RawVolume(region));
	delete _volumeRenderer.setVolume(ModelVolumeIndex, volume);
	delete _cursorVolumeRenderer.setVolume(CursorVolumeIndex, new voxel::RawVolume(region));

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

	setCursorShape(_shapeHandler.cursorShape());

	_dirty = false;
	_lastPlacement = glm::ivec3(-1);
	_result = voxel::PickResult();
	const glm::ivec3 pos = _cursorPos;
	_cursorPos = pos * 10 + 10;
	setCursorPosition(pos);
	const glm::ivec3 refPos(region.getCentreX(), 0, region.getCentreZ());
	setReferencePosition(refPos);
	resetLastTrace();
}

bool Model::newVolume(bool force) {
	if (dirty() && !force) {
		return false;
	}
	const voxel::Region region(glm::ivec3(0), glm::ivec3(size() - 1));
	undoHandler().clearUndoStates();
	setNewVolume(new voxel::RawVolume(region));
	modified(region);
	_dirty = false;
	return true;
}

void Model::rotate(int angleX, int angleY, int angleZ) {
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = voxel::rotateVolume(model, glm::vec3(angleX, angleY, angleZ), voxel::Voxel(), false);
	setNewVolume(newVolume);
	modified(newVolume->region());
}

void Model::move(int x, int y, int z) {
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = new voxel::RawVolume(model->region());
	voxel::RawVolumeMoveWrapper wrapper(newVolume);
	voxel::moveVolume(&wrapper, model, glm::ivec3(x, y, z));
	setNewVolume(newVolume);
	modified(newVolume->region());
}

static voxel::RawVolume* resample_(const voxel::RawVolume* srcVolume, const voxel::Region& srcRegion) {
	const glm::ivec3& lowerCorner = srcRegion.getLowerCorner();
	glm::ivec3 upperCorner = srcRegion.getUpperCorner();

	upperCorner = upperCorner - lowerCorner;
	upperCorner = upperCorner / static_cast<int32_t>(2);
	upperCorner = upperCorner + lowerCorner;

	const voxel::Region dstRegion(lowerCorner, upperCorner);
	if (!dstRegion.isValid()) {
		return nullptr;
	}
	voxel::RawVolume *dstVolume = new voxel::RawVolume(dstRegion);
	voxel::rescaleVolume(*srcVolume, srcRegion, *dstVolume, dstRegion);
	return dstVolume;
}

bool Model::resample(int factor) {
	if (!glm::isPowerOfTwo(factor)) {
		return false;
	}
	const voxel::RawVolume* model = modelVolume();
	voxel::Region srcRegion = model->region();
	srcRegion.grow(factor);

	voxel::RawVolume* newVolume = resample_(model, srcRegion);
	factor /= 2;
	while (factor > 1) {
		voxel::RawVolume* v = resample_(newVolume, newVolume->region());
		if (v == nullptr) {
			break;
		}
		delete newVolume;
		newVolume = v;
		factor /= 2;
	}

	setNewVolume(newVolume);
	modified(newVolume->region());
	return true;
}

const voxel::Voxel& Model::getVoxel(const glm::ivec3& pos) const {
	return modelVolume()->voxel(pos);
}

bool Model::setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) {
	voxel::RawVolumeWrapper wrapper(modelVolume());
	const bool placed = wrapper.setVoxel(pos, voxel);
	if (!placed) {
		return false;
	}
	_lastPlacement = pos;

	if (_mirrorAxis == math::Axis::None) {
		return true;
	}
	const int index = getIndexForMirrorAxis(_mirrorAxis);
	const int delta = _mirrorPos[index] - pos[index] - 1;
	if (delta == 0) {
		return true;
	}
	glm::ivec3 mirror;
	for (int i = 0; i < 3; ++i) {
		if (i == index) {
			mirror[i] = _mirrorPos[i] + delta;
		} else {
			mirror[i] = pos[i];
		}
	}
	wrapper.setVoxel(mirror, voxel);
	return true;
}

void Model::copy() {
	voxel::mergeRawVolumesSameDimension(cursorPositionVolume(), _selectionVolumeRenderer.volume());
	markCursorExtract();
}

void Model::paste() {
	voxel::RawVolume* cursorVolume = cursorPositionVolume();
	const voxel::Region& srcRegion = cursorVolume->region();
	const voxel::Region destRegion = srcRegion + _cursorPos;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::mergeVolumes(&wrapper, cursorVolume, destRegion, srcRegion);
}

void Model::cut() {
	voxel::RawVolume* cursorVolume = cursorPositionVolume();
	voxel::mergeRawVolumesSameDimension(cursorVolume, _selectionVolumeRenderer.volume(SelectionVolumeIndex));
	markCursorExtract();
	remove();
}

void Model::render(const video::Camera& camera) {
	const voxel::Mesh* mesh = _volumeRenderer.mesh(ModelVolumeIndex);
	_empty = mesh != nullptr ? mesh->getNoOfIndices() == 0 : true;
	_gridRenderer.render(camera, modelVolume()->region());
	_volumeRenderer.render(camera);
	_cursorVolumeRenderer.render(camera);
	if (_aabbMode) {
		_shapeBuilder.clear();
		_shapeBuilder.setColor(core::Color::Red);
		_shapeBuilder.cube(_aabbFirstPos, cursorPosition());
		_shapeRenderer.createOrUpdate(_aabbMeshIndex, _shapeBuilder);
		_shapeRenderer.render(_aabbMeshIndex, camera);
	}
	// TODO: render error if rendered last - but be before grid renderer to get transparency.
	if (_renderLockAxis) {
		for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
			_shapeRenderer.render(_planeMeshIndex[i], camera);
		}
	}
	_shapeRenderer.render(_mirrorMeshIndex, camera);
	renderSelection(camera);
}

void Model::renderSelection(const video::Camera& camera) {
	const voxel::Mesh* mesh = _selectionVolumeRenderer.mesh(SelectionVolumeIndex);
	if (mesh == nullptr || mesh->getNoOfIndices() == 0) {
		return;
	}
	video::ScopedPolygonMode polygonMode(video::PolygonMode::WireFrame, glm::vec2(-2.0f));
	video::ScopedLineWidth lineWidth(3.0f);
	video::ScopedBlendMode blendMode(video::BlendMode::One, video::BlendMode::One);
	_selectionVolumeRenderer.render(camera);
}

void Model::onResize(const glm::ivec2& size) {
	_volumeRenderer.onResize(glm::ivec2(0), size);
	_cursorVolumeRenderer.onResize(glm::ivec2(0), size);
	_selectionVolumeRenderer.onResize(glm::ivec2(0), size);
}

bool Model::init() {
	++_initialized;
	if (_initialized > 1) {
		return true;
	}
	_volumeRenderer.init();
	_cursorVolumeRenderer.init();
	_selectionVolumeRenderer.init();
	_shapeRenderer.init();
	_gridRenderer.init();

	_mirrorMeshIndex = -1;
	_aabbMeshIndex = -1;
	for (int i = 0; i < lengthof(_planeMeshIndex); ++i) {
		_planeMeshIndex[i] = -1;
	}

	_lastAction = Action::None;
	_action = Action::None;

	_lockedAxis = math::Axis::None;
	_mirrorAxis = math::Axis::None;
	return true;
}

void Model::update() {
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
	extractCursorVolume();
	extractSelectionVolume();
}

void Model::shutdown() {
	--_initialized;
	if (_initialized != 0) {
		return;
	}
	std::vector<voxel::RawVolume*> old = _volumeRenderer.shutdown();
	for (voxel::RawVolume* v : old) {
		delete v;
	}
	old = _cursorVolumeRenderer.shutdown();
	for (voxel::RawVolume* v : old) {
		delete v;
	}
	old = _selectionVolumeRenderer.shutdown();
	for (voxel::RawVolume* v : old) {
		delete v;
	}

	if (_spaceColonizationTree != nullptr) {
		delete _spaceColonizationTree;
		_spaceColonizationTree = nullptr;
	}

	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_gridRenderer.shutdown();
	undoHandler().clearUndoStates();
}

bool Model::extractSelectionVolume() {
	if (_extractSelection) {
		_extractSelection = false;
		_selectionVolumeRenderer.extract(SelectionVolumeIndex);
		return true;
	}
	return false;
}

bool Model::extractVolume() {
	if (_extract) {
		Log::debug("Extract the mesh");
		_extract = false;
		if (!_volumeRenderer.extract(ModelVolumeIndex)) {
			Log::error("Failed to extract the model mesh");
		}
		return true;
	}
	return false;
}

bool Model::extractCursorVolume() {
	if (_extractCursor) {
		_extractCursor = false;
		_cursorVolumeRenderer.extract(CursorVolumeIndex);
		return true;
	}
	return false;
}

void Model::noise(int octaves, float lacunarity, float frequency, float gain, voxel::noisegen::NoiseType type) {
	math::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::noisegen::generate(wrapper, octaves, lacunarity, frequency, gain, type, random);
	modified(wrapper.dirtyRegion());
}

void Model::spaceColonization() {
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

void Model::lsystem(const voxel::lsystem::LSystemContext& lsystemCtx) {
	math::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	if (voxel::lsystem::generate(wrapper, lsystemCtx, random)) {
		modified(wrapper.dirtyRegion());
	}
}

void Model::bezier(const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3& control) {
	voxel::RawVolumeWrapper wrapper(modelVolume());
	const int steps = glm::distance2(glm::vec3(start), glm::vec3(end)) * 10;
	voxel::shape::createBezier(wrapper,start, end, control, _shapeHandler.cursorVoxel(), steps);
	modified(wrapper.dirtyRegion());
}

void Model::createCactus() {
	math::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::cactus::createCactus(wrapper, _referencePos, 18, 2, random);
	modified(wrapper.dirtyRegion());
}

void Model::createCloud() {
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

void Model::createPlant(voxel::PlantType type) {
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

void Model::createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx) {
	voxel::RawVolumeWrapper wrapper(modelVolume());
	voxel::building::createBuilding(wrapper, _referencePos, type);
	modified(wrapper.dirtyRegion());
}

void Model::createTree(voxel::TreeContext ctx) {
	math::Random random;
	voxel::RawVolumeWrapper wrapper(modelVolume());
	ctx.pos = _referencePos;
	voxel::tree::createTree(wrapper, ctx, random);
	modified(wrapper.dirtyRegion());
}

void Model::setReferencePosition(const glm::ivec3& pos) {
	_referencePos = pos;
}

void Model::setCursorPosition(glm::ivec3 pos, bool force) {
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
	const voxel::Region& cursorRegion = cursorPositionVolume()->region();
	_cursorVolumeRenderer.setModelMatrix(CursorVolumeIndex, glm::translate(-cursorRegion.getCentre() + _cursorPos));

	updateLockedPlane(math::Axis::X);
	updateLockedPlane(math::Axis::Y);
	updateLockedPlane(math::Axis::Z);
}

void Model::markCursorExtract() {
	_extractCursor = true;
}

void Model::markExtract() {
	_extract = true;
}

bool Model::trace(const video::Camera& camera) {
	if (modelVolume() == nullptr) {
		return false;
	}

	if (_lastRaytraceX != _mouseX || _lastRaytraceY != _mouseY) {
		core_trace_scoped(EditorSceneOnProcessUpdateRay);
		_lastRaytraceX = _mouseX;
		_lastRaytraceY = _mouseY;

		const video::Ray& ray = camera.mouseRay(glm::ivec2(_mouseX, _mouseY));
		const glm::vec3& dirWithLength = ray.direction * camera.farPlane();
		static constexpr voxel::Voxel air;
		_result = voxel::pickVoxel(modelVolume(), ray.origin, dirWithLength, air);

		if (actionRequiresExistingVoxel(evalAction())) {
			if (_result.didHit) {
				setCursorPosition(_result.hitVoxel);
			} else if (_result.validPreviousPosition) {
				setCursorPosition(_result.previousPosition);
			}
		} else if (_result.validPreviousPosition) {
			setCursorPosition(_result.previousPosition);
		}
	}

	return true;
}

int Model::getIndexForAxis(math::Axis axis) const {
	if (axis == math::Axis::X) {
		return 0;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 2;
}

int Model::getIndexForMirrorAxis(math::Axis axis) const {
	if (axis == math::Axis::X) {
		return 2;
	} else if (axis == math::Axis::Y) {
		return 1;
	}
	return 0;
}

void Model::updateShapeBuilderForPlane(bool mirror, const glm::ivec3& pos, math::Axis axis, const glm::vec4& color) {
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

void Model::updateLockedPlane(math::Axis axis) {
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
	updateShapeBuilderForPlane(false, _cursorPos, axis, core::Color::alpha(colors[index], 0.3f));
	_shapeRenderer.createOrUpdate(meshIndex, _shapeBuilder);
}

math::Axis Model::mirrorAxis() const {
	return _mirrorAxis;
}

void Model::setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos) {
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

void Model::updateMirrorPlane() {
	if (_mirrorAxis == math::Axis::None) {
		if (_mirrorMeshIndex != -1) {
			_shapeRenderer.deleteMesh(_mirrorMeshIndex);
			_mirrorMeshIndex = -1;
		}
		return;
	}

	updateShapeBuilderForPlane(true, _mirrorPos, _mirrorAxis, core::Color::alpha(core::Color::LightGray, 0.1f));
	_shapeRenderer.createOrUpdate(_mirrorMeshIndex, _shapeBuilder);
}

void Model::setLockedAxis(math::Axis axis, bool unlock) {
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
