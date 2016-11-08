#include "Model.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "select/Single.h"

static const struct Selection {
	SelectType type;
	selections::Select& select;
} selectionsArray[] = {
	{SelectType::Single, selections::Single::get()},
	{SelectType::Same, selections::Single::get()},
	{SelectType::LineVertical, selections::Single::get()},
	{SelectType::LineHorizontal, selections::Single::get()},
	{SelectType::Edge, selections::Single::get()}
};
static_assert(SDL_arraysize(selectionsArray) == std::enum_value(SelectType::Max), "Array size doesn't match selection modes");

EditorModel::EditorModel() :
		_rawVolumeRenderer(true, false, true), _rawVolumeSelectionRenderer(false, false, false) {
	registerMoveCmd("+move_right", MOVERIGHT);
	registerMoveCmd("+move_left", MOVELEFT);
	registerMoveCmd("+move_forward", MOVEFORWARD);
	registerMoveCmd("+move_backward", MOVEBACKWARD);
}

EditorModel::~EditorModel() {
	shutdown();
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");
}

void EditorModel::select(const glm::ivec3& pos) {
	voxel::RawVolume* selectionVolume = _rawVolumeSelectionRenderer.volume();
	const Selection& mode = selectionsArray[std::enum_value(_selectionType)];
	if (mode.select.execute(_modelVolume, selectionVolume, pos)) {
		_selectionExtract = true;
	}
}

void EditorModel::executeAction(int32_t x, int32_t y, bool mouseDown, long now) {
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

	if (extract) {
		resetLastTrace();
	}

	if (extract) {
		_extract = true;
		_dirty = true;
	}
}

void EditorModel::resetLastTrace() {
	_lastRaytraceX = _lastRaytraceY = -1;
}

void EditorModel::setNewVolume(voxel::RawVolume* volume) {
	delete _modelVolume;
	_modelVolume = volume;

	const voxel::Region& region = volume->getEnclosingRegion();
	delete _cursorPositionVolume;
	_cursorPositionVolume = new voxel::RawVolume(region);

	delete _rawVolumeSelectionRenderer.setVolume(new voxel::RawVolume(region));
	delete _rawVolumeRenderer.setVolume(new voxel::RawVolume(region));

	_empty = true;
	_extract = true;
	_dirty = false;
	_lastRaytraceX = _lastRaytraceY = -1;
}

bool EditorModel::dirty() const {
	return _dirty;
}

float EditorModel::size() const {
	return _size;
}

bool EditorModel::empty() const {
	return _empty;
}

const voxel::Voxel& EditorModel::getVoxel(const glm::ivec3& pos) const {
	return _modelVolume->getVoxel(pos);
}

bool EditorModel::setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) const {
	return _modelVolume->setVoxel(pos, voxel);
}

void EditorModel::render(const video::Camera& camera) {
	_rawVolumeRenderer.render(camera);
}

void EditorModel::onResize(const glm::ivec2& pos, const glm::ivec2& size) {
	_rawVolumeRenderer.onResize(pos, size);
	_rawVolumeSelectionRenderer.onResize(pos, size);
}

void EditorModel::init() {
	if (_initialized++ > 0) {
		return;
	}
	_cursorVolume = new voxel::RawVolume(voxel::Region(0, 1));
	_cursorVolume->setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Grass1));
	_rawVolumeRenderer.init();
	_rawVolumeSelectionRenderer.init();
}

void EditorModel::shutdown() {
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
}

bool EditorModel::extractSelectionVolume() {
	if (_selectionExtract) {
		_selectionExtract = false;
		_rawVolumeSelectionRenderer.extract();
		return true;
	}
	return false;
}

bool EditorModel::extractVolume() {
	if (_extract) {
		_extract = false;
		_rawVolumeRenderer.extract();
		return true;
	}
	return false;
}

void EditorModel::trace(int mouseX, int mouseY, bool skipCursor, const video::Camera& camera) {
	if (_lastRaytraceX != mouseX || _lastRaytraceY != mouseY) {
		core_trace_scoped(EditorSceneOnProcessUpdateRay);
		_lastRaytraceX = mouseX;
		_lastRaytraceY = mouseY;

		const int tx = mouseX;
		const int ty = mouseY;
		const video::Ray& ray = camera.mouseRay(glm::ivec2(tx, ty));
		const glm::vec3& dirWithLength = ray.direction * camera.farPlane();
		const voxel::Voxel& air = voxel::createVoxel(voxel::VoxelType::Air);
		_result = voxel::pickVoxel(modelVolume(), ray.origin, dirWithLength, air);

		if (_result.validPreviousVoxel && (!_result.didHit || !actionRequiresExistingVoxel(action()))) {
			_cursorPositionVolume->clear();
			const glm::ivec3& center = _cursorVolume->getEnclosingRegion().getCentre();
			const glm::ivec3& cursorPos = _result.previousVoxel - center;
			voxel::mergeRawVolumes(_cursorPositionVolume, _cursorVolume, cursorPos);
		} else if (_result.didHit) {
			_cursorPositionVolume->clear();
			const glm::ivec3& center = _cursorVolume->getEnclosingRegion().getCentre();
			const glm::ivec3& cursorPos = _result.previousVoxel - center;
			voxel::mergeRawVolumes(_cursorPositionVolume, _cursorVolume, cursorPos);
			_cursorPositionVolume->setVoxel(_result.hitVoxel, currentVoxel());
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
}
