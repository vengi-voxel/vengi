#include "Model.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/VolumeCropper.h"
#include "voxel/polyvox/VolumeRotator.h"
#include "voxel/model/VoxFormat.h"
#include "voxel/model/QB2Format.h"
#include "select/Edge.h"
#include "select/LineHorizontal.h"
#include "select/LineVertical.h"
#include "select/Same.h"
#include "select/Single.h"

namespace voxedit {

static const struct Selection {
	SelectType type;
	selections::Select& select;
} selectionsArray[] = {
	{SelectType::Single,			selections::Single::get()},
	{SelectType::Same,				selections::Same::get()},
	{SelectType::LineVertical,		selections::LineVertical::get()},
	{SelectType::LineHorizontal,	selections::LineHorizontal::get()},
	{SelectType::Edge,				selections::Edge::get()}
};
static_assert(SDL_arraysize(selectionsArray) == std::enum_value(SelectType::Max), "Array size doesn't match selection modes");

Model::Model() :
		_rawVolumeRenderer(true, false, true), _rawVolumeSelectionRenderer(false, false, false) {
	_undoStates.reserve(_maxUndoStates);
}

Model::~Model() {
	shutdown();
}

bool Model::save(std::string_view file) {
	if (!dirty()) {
		// nothing to save yet
		return true;
	}
	if (modelVolume() == nullptr) {
		return false;
	}
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	voxel::VoxFormat f;
	if (f.save(modelVolume(), filePtr)) {
		_dirty = false;
	}
	return !dirty();
}

bool Model::load(std::string_view file) {
	const io::FilePtr& filePtr = core::App::getInstance()->filesystem()->open(std::string(file));
	if (!(bool)filePtr) {
		Log::error("Failed to open model file %s", file.data());
		return false;
	}
	voxel::VoxFormat f;
	voxel::RawVolume* newVolume = f.load(filePtr);
	if (newVolume == nullptr) {
		Log::error("Failed to load model file %s", file.data());
		return false;
	}
	Log::info("Loaded model file %s", file.data());
	clearUndoStates();
	setNewVolume(newVolume);
	return true;
}

void Model::select(const glm::ivec3& pos) {
	voxel::RawVolume* selectionVolume = _rawVolumeSelectionRenderer.volume();
	const Selection& mode = selectionsArray[std::enum_value(_selectionType)];
	// TODO: unselect
	if (mode.select.execute(_modelVolume, selectionVolume, pos)) {
		_selectionExtract = true;
	}
}

void Model::unselectAll() {
	_rawVolumeSelectionRenderer.volume()->clear();
	_selectionExtract = true;
}

void Model::setMousePos(int x, int y) {
	_mouseX = x;
	_mouseY = y;
}

void Model::crop() {
	if (_empty) {
		Log::info("Empty volumes can't be cropped");
		return;
	}
	voxel::RawVolume* newVolume = voxel::cropVolume(_modelVolume, voxel::createVoxel(voxel::VoxelType::Air));
	if (newVolume == nullptr) {
		Log::info("Failed to crop the model volume");
		return;
	}
	const glm::ivec3& oldMaxs = _modelVolume->getEnclosingRegion().getUpperCorner();
	const glm::ivec3& newMaxs = newVolume->getEnclosingRegion().getUpperCorner();
	const glm::ivec3 delta = oldMaxs - newMaxs;
	voxel::mergeRawVolumes(newVolume, _modelVolume, delta);
	markUndo();
	setNewVolume(newVolume);
}

void Model::extend(int size) {
	voxel::Region region = _modelVolume->getEnclosingRegion();
	region.grow(size);
	if (!region.isValid()) {
		return;
	}
	voxel::RawVolume* newVolume = new voxel::RawVolume(region);
	voxel::mergeRawVolumes(newVolume, _modelVolume, glm::ivec3(size));
	markUndo();
	setNewVolume(newVolume);
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
	const glm::ivec3& hitVoxel = _result.hitVoxel;
	const bool didHit = _result.didHit;
	if (didHit && _action == Action::CopyVoxel) {
		setVoxelType(getVoxel(hitVoxel).getMaterial());
	} else if (didHit && _action == Action::SelectVoxels) {
		select(hitVoxel);
	} else if (didHit && _action == Action::OverrideVoxel) {
		extract = setVoxel(hitVoxel, _currentVoxel);
	} else if (didHit && _action == Action::DeleteVoxel) {
		extract = setVoxel(hitVoxel, voxel::createVoxel(voxel::VoxelType::Air));
	} else if (_result.validPreviousVoxel && _action == Action::PlaceVoxel) {
		extract = setVoxel(_result.previousVoxel, _currentVoxel);
	} else if (didHit && _action == Action::PlaceVoxel) {
		extract = setVoxel(hitVoxel, _currentVoxel);
	}

	if (!extract) {
		return;
	}
	resetLastTrace();
	_extract = true;
	_dirty = true;
}

void Model::resetLastTrace() {
	_lastRaytraceX = _lastRaytraceY = -1;
}

void Model::setNewVolume(voxel::RawVolume* volume) {
	delete _modelVolume;
	_modelVolume = volume;

	const voxel::Region& region = volume->getEnclosingRegion();
	delete _cursorPositionVolume;
	_cursorPositionVolume = new voxel::RawVolume(region);

	delete _cursorVolume;
	_cursorVolume = new voxel::RawVolume(region);

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
	clearUndoStates();
	setNewVolume(new voxel::RawVolume(region));
	return true;
}

void Model::rotate(int angleX, int angleY, int angleZ) {
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = voxel::rotateVolume(model, glm::vec3(angleX, angleY, angleZ), false);
	markUndo();
	setNewVolume(newVolume);
}

const voxel::Voxel& Model::getVoxel(const glm::ivec3& pos) const {
	return _modelVolume->getVoxel(pos);
}

bool Model::setVoxel(glm::ivec3 pos, const voxel::Voxel& voxel) {
	if (getVoxel(pos) == voxel) {
		return false;
	}
	if ((_lockedAxis & Axis::X) != Axis::None) {
		if (_lastPlacement.x >= 0) {
			pos.x = _lastPlacement.x;
		}
	}
	if ((_lockedAxis & Axis::Y) != Axis::None) {
		if (_lastPlacement.y >= 0) {
			pos.y = _lastPlacement.y;
		}
	}
	if ((_lockedAxis & Axis::Z) != Axis::None) {
		if (_lastPlacement.z >= 0) {
			pos.z = _lastPlacement.z;
		}
	}
	markUndo();
	const bool placed = _modelVolume->setVoxel(pos, voxel);
	_lastPlacement = pos;
	return placed;
}

void Model::copy() {
	voxel::mergeRawVolumes(_cursorVolume, _rawVolumeSelectionRenderer.volume(), glm::ivec3(0));
}

void Model::paste() {
	voxel::mergeRawVolumes(_modelVolume, _cursorVolume, _cursorPos);
}

void Model::cut() {
	voxel::mergeRawVolumes(_cursorVolume, _rawVolumeSelectionRenderer.volume(), glm::ivec3(0));
	// TODO: delete selected volume from model volume
}

void Model::undo() {
	if (!canUndo()) {
		return;
	}
	setNewVolume(new voxel::RawVolume(_undoStates[--_undoIndex]));
}

void Model::redo() {
	// nothing to redo
	if (!canRedo()) {
		return;
	}
	setNewVolume(new voxel::RawVolume(_undoStates[++_undoIndex]));
}

void Model::markUndo() {
	auto i = _undoStates.begin();
	std::advance(i, _undoIndex);
	for (auto iter = i; iter < _undoStates.end(); ++iter) {
		delete *iter;
	}
	_undoStates.erase(i, _undoStates.end());
	_undoStates.push_back(new voxel::RawVolume(_modelVolume));
	while (_undoStates.size() > _maxUndoStates) {
		delete *_undoStates.begin();
		_undoStates.erase(_undoStates.begin());
	}
	_undoIndex = _undoStates.size();
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
	clearUndoStates();
}

void Model::clearUndoStates() {
	for (voxel::RawVolume* vol : _undoStates) {
		delete vol;
	}
	_undoStates.clear();
	_undoIndex = 0u;
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

bool Model::trace(bool skipCursor, const video::Camera& camera) {
	if (_modelVolume == nullptr) {
		return false;
	}

	if (_lastRaytraceX != _mouseX || _lastRaytraceY != _mouseY) {
		core_trace_scoped(EditorSceneOnProcessUpdateRay);
		_lastRaytraceX = _mouseX;
		_lastRaytraceY = _mouseY;

		const video::Ray& ray = camera.mouseRay(glm::ivec2(_mouseX, _mouseY));
		const glm::vec3& dirWithLength = ray.direction * camera.farPlane();
		const voxel::Voxel& air = voxel::createVoxel(voxel::VoxelType::Air);
		_result = voxel::pickVoxel(modelVolume(), ray.origin, dirWithLength, air);

		if (!skipCursor) {
			if (_result.validPreviousVoxel && (!_result.didHit || !actionRequiresExistingVoxel(action()))) {
				_cursorPositionVolume->clear();
				const glm::ivec3& center = _cursorVolume->getEnclosingRegion().getCentre();
				const glm::ivec3& cursorPos = _result.previousVoxel - center;
				// TODO
				const voxel::Region srcRegion = _cursorVolume->getEnclosingRegion();
				const voxel::Region destRegion = srcRegion + cursorPos;
				voxel::mergeRawVolumes(_cursorPositionVolume, _cursorVolume, destRegion, srcRegion);
				_cursorPos = cursorPos;
			} else if (_result.didHit) {
				_cursorPositionVolume->clear();
				const glm::ivec3& center = _cursorVolume->getEnclosingRegion().getCentre();
				const glm::ivec3& cursorPos = _result.hitVoxel - center;
				// TODO
				const voxel::Region srcRegion;
				const voxel::Region destRegion;
				voxel::mergeRawVolumes(_cursorPositionVolume, _cursorVolume, destRegion, srcRegion);
				_cursorPositionVolume->setVoxel(_result.hitVoxel, currentVoxel());
				_cursorPos = cursorPos;
			}
		}

		core_trace_scoped(EditorSceneOnProcessMergeRawVolumes);
		voxel::RawVolume* volume = rawVolumeRenderer().volume();
		volume->clear();
		if (!skipCursor) {
			voxel::mergeRawVolumesSameDimension(volume, _cursorPositionVolume);
		}
		_empty = voxel::mergeRawVolumesSameDimension(volume, modelVolume()) == 0;
		_extract = true;
	}

	extractVolume();
	extractSelectionVolume();

	return true;
}

void Model::setCursorShape(Shape type) {
	if (_cursorShape == type) {
		return;
	}
	_cursorShape = type;
	_cursorShapeState = CursorShapeState::New;
	if (_cursorShape == Shape::Single) {
		_cursorVolume->setVoxel(_cursorVolume->getEnclosingRegion().getCentre(), _currentVoxel);
		_cursorShapeState = CursorShapeState::Created;
	}
}

}
