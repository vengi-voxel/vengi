#include "Model.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "select/Edge.h"
#include "select/LineHorizontal.h"
#include "select/LineVertical.h"
#include "select/Same.h"
#include "select/Single.h"
#include "voxel/model/VoxFormat.h"
#include "voxel/model/QB2Format.h"

namespace voxedit {

static const struct Selection {
	SelectType type;
	selections::Select& select;
} selectionsArray[] = {
	{SelectType::Single, selections::Single::get()},
	{SelectType::Same, selections::Same::get()},
	{SelectType::LineVertical, selections::LineVertical::get()},
	{SelectType::LineHorizontal, selections::LineHorizontal::get()},
	{SelectType::Edge, selections::Edge::get()}
};
static_assert(SDL_arraysize(selectionsArray) == std::enum_value(SelectType::Max), "Array size doesn't match selection modes");

Model::Model() :
		_rawVolumeRenderer(true, false, true), _rawVolumeSelectionRenderer(false, false, false) {
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
	setNewVolume(newVolume);
	return true;
}

void Model::select(const glm::ivec3& pos) {
	voxel::RawVolume* selectionVolume = _rawVolumeSelectionRenderer.volume();
	const Selection& mode = selectionsArray[std::enum_value(_selectionType)];
	if (mode.select.execute(_modelVolume, selectionVolume, pos)) {
		_selectionExtract = true;
	}
}

void Model::setMousePos(int x, int y) {
	_mouseX = x;
	_mouseY = y;
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

	delete _rawVolumeSelectionRenderer.setVolume(new voxel::RawVolume(region));
	delete _rawVolumeRenderer.setVolume(new voxel::RawVolume(region));

	_empty = true;
	_extract = true;
	_dirty = false;
	resetLastTrace();
}

bool Model::newVolume(bool force) {
	if (dirty() && !force) {
		return false;
	}
	_dirty = false;
	_result = voxel::PickResult();
	_extract = true;
	resetLastTrace();

	const voxel::Region region(glm::ivec3(0), glm::ivec3(size()));
	setNewVolume(new voxel::RawVolume(region));

	return true;
}

bool Model::dirty() const {
	return _dirty;
}

float Model::size() const {
	return _size;
}

bool Model::empty() const {
	return _empty;
}

const voxel::Voxel& Model::getVoxel(const glm::ivec3& pos) const {
	return _modelVolume->getVoxel(pos);
}

bool Model::setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel) const {
	Log::debug("Set voxel %i to v(%i:%i:%i)", std::enum_value(voxel.getMaterial()), pos.x, pos.y, pos.z);
	return _modelVolume->setVoxel(pos, voxel);
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
	// TODO: shapes
	_cursorVolume = new voxel::RawVolume(voxel::Region(0, 1));
	_cursorVolume->setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Grass1));
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
				voxel::mergeRawVolumes(_cursorPositionVolume, _cursorVolume, cursorPos);
			} else if (_result.didHit) {
				_cursorPositionVolume->clear();
				const glm::ivec3& center = _cursorVolume->getEnclosingRegion().getCentre();
				const glm::ivec3& cursorPos = _result.previousVoxel - center;
				voxel::mergeRawVolumes(_cursorPositionVolume, _cursorVolume, cursorPos);
				_cursorPositionVolume->setVoxel(_result.hitVoxel, currentVoxel());
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

}
