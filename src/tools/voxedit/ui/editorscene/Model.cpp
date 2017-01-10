#include "Model.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/VolumeCropper.h"
#include "voxel/polyvox/VolumeRotator.h"
#include "voxel/polyvox/VolumeMover.h"
#include "voxel/polyvox/VolumeRescaler.h"
#include "voxel/polyvox//RawVolumeWrapper.h"
#include "voxel/polyvox//RawVolumeMoveWrapper.h"
#include "voxel/generator/NoiseGenerator.h"
#include "voxel/generator/WorldGenerator.h"
#include "voxel/generator/CloudGenerator.h"
#include "voxel/generator/CactusGenerator.h"
#include "voxel/generator/BuildingGenerator.h"
#include "voxel/model/VoxFormat.h"
#include "voxel/model/QBTFormat.h"
#include "voxel/model/QBFormat.h"
#include "tool/Crop.h"
#include "tool/Expand.h"
#include "core/Random.h"
#include "tool/Fill.h"

namespace voxedit {

Model::Model() :
		_rawVolumeRenderer(true, false, true), _rawVolumeSelectionRenderer(false, false, false) {
}

Model::~Model() {
	shutdown();
}

bool Model::save(std::string_view file) {
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

bool Model::load(std::string_view file) {
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
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
		Log::error("Failed to load model file %s", file.data());
		return false;
	}
	Log::info("Loaded model file %s", file.data());
	_undoHandler.clearUndoStates();
	setNewVolume(newVolume);
	return true;
}

void Model::select(const glm::ivec3& pos) {
	voxel::RawVolume* selectionVolume = _rawVolumeSelectionRenderer.volume();
	_selectionExtract |= _selectionHandler.select(_modelVolume, selectionVolume, pos);
}

void Model::unselectAll() {
	_selectionHandler.unselectAll();
	_rawVolumeSelectionRenderer.volume()->clear();
	_selectionExtract = true;
}

void Model::setMousePos(int x, int y) {
	_mouseX = x;
	_mouseY = y;
}

void Model::markUndo() {
	_undoHandler.markUndo(_modelVolume);
}

void Model::crop() {
	if (_empty) {
		Log::info("Empty volumes can't be cropped");
		return;
	}
	voxel::RawVolume* newVolume = voxedit::tool::crop(_modelVolume);
	if (newVolume == nullptr) {
		return;
	}
	markUndo();
	setNewVolume(newVolume);
}

void Model::extend(int size) {
	voxel::RawVolume* newVolume = voxedit::tool::expand(_modelVolume, size);
	if (newVolume == nullptr) {
		return;
	}
	markUndo();
	setNewVolume(newVolume);
}

void Model::scale() {
	// TODO: check that src region boundaries are even
	const voxel::Region& srcRegion = _modelVolume->getRegion();
	const int w = srcRegion.getWidthInVoxels();
	const int h = srcRegion.getHeightInVoxels();
	const int d = srcRegion.getDepthInVoxels();
	const glm::ivec3 maxs(w / 2, h / 2, d / 2);
	voxel::Region region(glm::zero<glm::ivec3>(), maxs);
	voxel::RawVolume* newVolume = new voxel::RawVolume(region);
	voxel::RawVolumeWrapper wrapper(newVolume);
	voxel::rescaleVolume(*_modelVolume, *newVolume);
	markUndo();
	setNewVolume(newVolume);
}

void Model::fill(int x, int y, int z) {
	markUndo();
	voxedit::tool::fill(*_modelVolume, glm::ivec3(x, y, z), _lockedAxis, _shapeHandler.currentVoxel());
}

void Model::executeAction(bool mouseDown, long now) {
	if (_action == Action::None || !mouseDown) {
		return;
	}

	core_trace_scoped(EditorSceneExecuteAction);
	if (_lastAction == _action) {
		if (now - _lastActionExecution < _actionExecutionDelay) {
			return;
		}
	}
	_lastAction = _action;
	_lastActionExecution = now;

	bool extract = false;
	const bool didHit = _result.didHit;
	if (didHit && _action == Action::CopyVoxel) {
		shapeHandler().setVoxel(getVoxel(_cursorPos));
	} else if (didHit && _action == Action::SelectVoxels) {
		select(_cursorPos);
	} else if (didHit && _action == Action::OverrideVoxel) {
		extract = placeCursor();
	} else if (didHit && _action == Action::DeleteVoxel) {
		extract = setVoxel(_cursorPos, voxel::Voxel());
	} else if (_result.validPreviousVoxel && _action == Action::PlaceVoxel) {
		extract = placeCursor();
	} else if (didHit && _action == Action::PlaceVoxel) {
		extract = placeCursor();
	}

	if (!extract) {
		return;
	}
	resetLastTrace();
	_extract = true;
	_dirty = true;
}

void Model::undo() {
	voxel::RawVolume* v = _undoHandler.undo();
	if (v == nullptr) {
		return;
	}
	setNewVolume(v);
}

void Model::redo() {
	voxel::RawVolume* v = _undoHandler.redo();
	if (v == nullptr) {
		return;
	}
	setNewVolume(v);
}

bool Model::placeCursor() {
	return _shapeHandler.placeCursor(_modelVolume, _cursorPositionVolume);
}

void Model::resetLastTrace() {
	_lastRaytraceX = _lastRaytraceY = -1;
}

void Model::setNewVolume(voxel::RawVolume* volume) {
	delete _modelVolume;
	_modelVolume = volume;

	const voxel::Region& region = volume->getRegion();
	delete _cursorPositionVolume;
	_cursorPositionVolume = new voxel::RawVolume(region);

	delete _cursorVolume;
	_cursorVolume = new voxel::RawVolume(region);
	setCursorShape(Shape::Single);

	delete _rawVolumeSelectionRenderer.setVolume(new voxel::RawVolume(region));
	delete _rawVolumeRenderer.setVolume(new voxel::RawVolume(region));

	_empty = true;
	_extract = true;
	_dirty = false;
	_lastPlacement = glm::ivec3(-1);
	_result = voxel::PickResult();
	resetLastTrace();
}

bool Model::newVolume(bool force) {
	if (dirty() && !force) {
		return false;
	}
	const voxel::Region region(glm::ivec3(0), glm::ivec3(size() - 1));
	_undoHandler.clearUndoStates();
	setNewVolume(new voxel::RawVolume(region));
	return true;
}

void Model::rotate(int angleX, int angleY, int angleZ) {
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = voxel::rotateVolume(model, glm::vec3(angleX, angleY, angleZ), voxel::Voxel(), false);
	markUndo();
	setNewVolume(newVolume);
}

void Model::move(int x, int y, int z) {
	voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = new voxel::RawVolume(model->getRegion());
	voxel::RawVolumeMoveWrapper wrapper(newVolume);
	voxel::moveVolume(&wrapper, model, glm::ivec3(x, y, z), voxel::Voxel());
	markUndo();
	setNewVolume(newVolume);
}

const voxel::Voxel& Model::getVoxel(const glm::ivec3& pos) const {
	return _modelVolume->getVoxel(pos);
}

bool Model::setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) {
	if (getVoxel(pos) == voxel) {
		return false;
	}
	markUndo();
	const bool placed = _modelVolume->setVoxel(pos, voxel);
	_lastPlacement = pos;
	return placed;
}

void Model::copy() {
	voxel::mergeRawVolumesSameDimension(_cursorVolume, _rawVolumeSelectionRenderer.volume());
}

void Model::paste() {
	const voxel::Region& srcRegion = _cursorVolume->getRegion();
	const voxel::Region destRegion = srcRegion + _cursorPos;
	voxel::RawVolumeWrapper wrapper(_modelVolume);
	voxel::mergeRawVolumes(&wrapper, _cursorVolume, destRegion, srcRegion);
}

void Model::cut() {
	voxel::mergeRawVolumesSameDimension(_cursorVolume, _rawVolumeSelectionRenderer.volume());
	// TODO: delete selected volume from model volume
}

void Model::render(const video::Camera& camera) {
	_rawVolumeRenderer.render(camera);
}

void Model::renderSelection(const video::Camera& camera) {
	_rawVolumeSelectionRenderer.render(camera);
}

void Model::onResize(const glm::ivec2& size) {
	_rawVolumeRenderer.onResize(glm::ivec2(), size);
	_rawVolumeSelectionRenderer.onResize(glm::ivec2(), size);
}

void Model::init() {
	if (_initialized++ > 0) {
		return;
	}
	_rawVolumeRenderer.init();
	_rawVolumeSelectionRenderer.init();
}

void Model::shutdown() {
	if (--_initialized > 0) {
		return;
	}
	delete _cursorPositionVolume;
	_cursorPositionVolume = nullptr;
	delete _cursorVolume;
	_cursorVolume = nullptr;
	delete _modelVolume;
	_modelVolume = nullptr;
	delete _rawVolumeRenderer.shutdown();
	delete _rawVolumeSelectionRenderer.shutdown();
	_undoHandler.clearUndoStates();
}

bool Model::extractSelectionVolume() {
	if (_selectionExtract) {
		_selectionExtract = false;
		_rawVolumeSelectionRenderer.extract();
		return true;
	}
	return false;
}

bool Model::extractVolume() {
	if (_extract) {
		_extract = false;
		_rawVolumeRenderer.extract();
		return true;
	}
	return false;
}

void Model::noise(int octaves, float frequency, float persistence) {
	core::Random random;
	voxel::RawVolumeWrapper wrapper(_modelVolume);
	markUndo();
	voxel::noise::generate(wrapper, octaves, frequency, persistence, random);
}

void Model::lsystem(const voxel::lsystem::LSystemContext& lsystemCtx) {
	core::Random random;
	voxel::RawVolumeWrapper wrapper(_modelVolume);
	markUndo();
	voxel::lsystem::generate(wrapper, lsystemCtx, random);
}

void Model::world(const voxel::WorldContext& ctx) {
	markUndo();
	const voxel::Region region(glm::ivec3(0), glm::ivec3(127, 63, 127));
	setNewVolume(new voxel::RawVolume(region));
	voxel::BiomeManager mgr;
	mgr.init();
	voxel::RawVolumeWrapper wrapper(_modelVolume);
	voxel::world::createWorld(ctx, wrapper, mgr, 1L, voxel::world::WORLDGEN_CLIENT, 0, 0);
}

void Model::createCactus() {
	core::Random random;
	voxel::RawVolumeWrapper wrapper(_modelVolume);
	markUndo();
	voxel::cactus::createCactus(wrapper, _cursorPos, 18, 2, random);
}

void Model::createCloud() {
	core::Random random;
	voxel::RawVolumeWrapper wrapper(_modelVolume);
	struct HasClouds {
		inline bool hasClouds(const glm::ivec3&) const {
			return true;
		}
	};
	HasClouds hasClouds;
	voxel::cloud::CloudContext cloudCtx;
	cloudCtx.amount = 1;
	cloudCtx.regionBorder = 2;
	cloudCtx.randomPos = false;
	cloudCtx.pos = _cursorPos;
	markUndo();
	voxel::cloud::createClouds(wrapper, hasClouds, cloudCtx, random);
}

void Model::createPlant(voxel::PlantType type) {
	voxel::PlantGenerator g;
	voxel::RawVolumeWrapper wrapper(_modelVolume);
	markUndo();
	if (type == voxel::PlantType::Flower) {
		g.createFlower(5, _cursorPos, wrapper);
	} else if (type == voxel::PlantType::Grass) {
		g.createGrass(10, _cursorPos, wrapper);
	} else if (type == voxel::PlantType::Mushroom) {
		g.createMushroom(7, _cursorPos, wrapper);
	}
	g.shutdown();
}

void Model::createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx) {
	core::Random random;
	voxel::RawVolumeWrapper wrapper(_modelVolume);
	markUndo();
	voxel::building::createBuilding(wrapper, _cursorPos, type, random);
}

void Model::createTree(voxel::TreeContext ctx) {
	core::Random random;
	voxel::RawVolumeWrapper wrapper(_modelVolume);
	ctx.pos = _cursorPos;
	markUndo();
	voxel::tree::createTree(wrapper, ctx, random);
}

void Model::setCursorPosition(glm::ivec3 pos, bool force) {
	if (!force) {
		if ((_lockedAxis & Axis::X) != Axis::None) {
			pos.x = _cursorPos.x;
		}
		if ((_lockedAxis & Axis::Y) != Axis::None) {
			pos.y = _cursorPos.y;
		}
		if ((_lockedAxis & Axis::Z) != Axis::None) {
			pos.z = _cursorPos.z;
		}
	}

	const voxel::Region& region = _modelVolume->getRegion();
	if (!region.containsPoint(pos)) {
		pos = region.moveInto(pos.x, pos.y, pos.z);
	}
	_cursorPos = pos;

	_cursorPositionVolume->clear();
	static constexpr voxel::Voxel air;
	const std::unique_ptr<voxel::RawVolume> cropped(voxel::cropVolume(_cursorVolume, air));
	if (cropped) {
		const voxel::Region& srcRegion = cropped->getRegion();
		const voxel::Region& destRegion = _cursorPositionVolume->getRegion();
		const glm::ivec3& lower = destRegion.getLowerCorner() + _cursorPos - srcRegion.getCentre();
		if (destRegion.containsPoint(lower)) {
			const glm::ivec3& regionUpperCorner = destRegion.getUpperCorner();
			glm::ivec3 upper = lower + srcRegion.getDimensionsInVoxels();
			if (!destRegion.containsPoint(upper)) {
				upper = regionUpperCorner;
			}
			voxel::RawVolumeWrapper wrapper(_cursorPositionVolume);
			voxel::mergeRawVolumes(&wrapper, cropped.get(), voxel::Region(lower, upper), srcRegion);
		}
	} else {
		Log::error("Failed to crop cursor volume");
	}

	voxel::RawVolume* volume = rawVolumeRenderer().volume();
	volume->clear();
	voxel::mergeRawVolumesSameDimension(volume, _cursorPositionVolume);
	_empty = voxel::mergeRawVolumesSameDimension(volume, modelVolume()) == 0;
	_extract = true;
}

bool Model::trace(const video::Camera& camera) {
	if (_modelVolume == nullptr) {
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

		const bool prevVoxel = _result.validPreviousVoxel && (!_result.didHit || !actionRequiresExistingVoxel(action()));
		const bool directVoxel = _result.didHit;
		glm::ivec3 cursorPos;
		if (prevVoxel) {
			cursorPos = _result.previousVoxel;
		} else if (directVoxel) {
			cursorPos = _result.hitVoxel;
		}

		core_trace_scoped(EditorSceneOnProcessMergeRawVolumes);
		voxel::RawVolume* volume = rawVolumeRenderer().volume();
		volume->clear();
		if (prevVoxel || directVoxel) {
			setCursorPosition(cursorPos);
		} else {
			voxel::RawVolume* volume = rawVolumeRenderer().volume();
			volume->clear();
			_empty = voxel::mergeRawVolumesSameDimension(volume, modelVolume()) == 0;
			_extract = true;
		}

		voxel::mergeRawVolumesSameDimension(volume, _cursorPositionVolume);
	}

	extractVolume();
	extractSelectionVolume();

	return true;
}

}
