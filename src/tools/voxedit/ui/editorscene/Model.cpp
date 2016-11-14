#include "Model.h"
#include "voxel/polyvox/VolumeMerger.h"
#include "voxel/polyvox/VolumeCropper.h"
#include "voxel/polyvox/VolumeRotator.h"
#include "voxel/generator/RawVolumeWrapper.h"
#include "voxel/model/VoxFormat.h"
#include "voxel/model/QB2Format.h"
#include "tool/Crop.h"
#include "tool/Expand.h"
#include "core/Random.h"

namespace voxedit {

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
	_undoHandler.clearUndoStates();
	setNewVolume(newVolume);
	return true;
}

void Model::select(const glm::ivec3& pos) {
	voxel::RawVolume* selectionVolume = _rawVolumeSelectionRenderer.volume();
	_selectionExtract |= _selectionHandler.select(_modelVolume, selectionVolume, pos);
}

void Model::unselectAll() {
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
		shapeHandler().setVoxelType(getVoxel(_cursorPos).getMaterial());
	} else if (didHit && _action == Action::SelectVoxels) {
		select(_cursorPos);
	} else if (didHit && _action == Action::OverrideVoxel) {
		extract = placeCursor();
	} else if (didHit && _action == Action::DeleteVoxel) {
		extract = setVoxel(_cursorPos, voxel::createVoxel(voxel::VoxelType::Air));
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

	const voxel::Region& region = volume->getEnclosingRegion();
	delete _cursorPositionVolume;
	_cursorPositionVolume = new voxel::RawVolume(region);

	delete _cursorVolume;
	_cursorVolume = new voxel::RawVolume(region);
	core_assert_always(_shapeHandler.setCursorShape(Shape::Single, _cursorVolume, true));

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
	voxel::RawVolume* newVolume = voxel::rotateVolume(model, glm::vec3(angleX, angleY, angleZ), voxel::createVoxel(voxel::VoxelType::Air), false);
	markUndo();
	setNewVolume(newVolume);
}

void Model::move(int x, int y, int z) {
	// TODO: implement move
#if 0
	const voxel::RawVolume* model = modelVolume();
	voxel::RawVolume* newVolume = voxel::rotateVolume(model, glm::vec3(angleX, angleY, angleZ), voxel::createVoxel(voxel::VoxelType::Air), false);
	markUndo();
	setNewVolume(newVolume);
#endif
}

const voxel::Voxel& Model::getVoxel(const glm::ivec3& pos) const {
	return _modelVolume->getVoxel(pos);
}

bool Model::setVoxel(glm::ivec3 pos, const voxel::Voxel& voxel) {
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
	const voxel::Region& srcRegion = _cursorVolume->getEnclosingRegion();
	const voxel::Region destRegion = srcRegion + _cursorPos;
	voxel::mergeRawVolumes(_modelVolume, _cursorVolume, destRegion, srcRegion);
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

void Model::lsystem(const voxel::LSystemGenerator::LSystemContext& lsystemCtx) {
	core::Random random;
	voxel::generate::RawVolumeWrapper wrapper(_modelVolume);
	voxel::LSystemGenerator::generate(wrapper, lsystemCtx, random);
}

void Model::createTree(voxel::TreeType type) {
	core::Random random;
	const voxel::Region& region = _modelVolume->getEnclosingRegion();
	glm::ivec3 cursorPos = region.getCentre();
	cursorPos.y = region.getLowerY();
	voxel::generate::RawVolumeWrapper wrapper(_modelVolume);
	voxel::TreeContext ctx;
	ctx.type = type;
	ctx.pos = cursorPos;
	voxel::tree::createTree(wrapper, ctx, random);
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
			const bool prevVoxel = _result.validPreviousVoxel && (!_result.didHit || !actionRequiresExistingVoxel(action()));
			const bool directVoxel = _result.didHit;
			glm::ivec3 cursorPos;
			if (prevVoxel) {
				cursorPos = _result.previousVoxel;
			} else if (directVoxel) {
				cursorPos = _result.hitVoxel;
			}

			if ((_lockedAxis & Axis::X) != Axis::None) {
				cursorPos.x = _cursorPos.x;
			}
			if ((_lockedAxis & Axis::Y) != Axis::None) {
				cursorPos.y = _cursorPos.y;
			}
			if ((_lockedAxis & Axis::Z) != Axis::None) {
				cursorPos.z = _cursorPos.z;
			}
			_cursorPos = cursorPos;

			if (prevVoxel || directVoxel) {
				_cursorPositionVolume->clear();
				const std::unique_ptr<voxel::RawVolume> cropped(voxel::cropVolume(_cursorVolume, air));
				if (cropped) {
					const voxel::Region& srcRegion = cropped->getEnclosingRegion();
					const voxel::Region& destRegion = _cursorPositionVolume->getEnclosingRegion();
					const glm::ivec3& lower = destRegion.getLowerCorner() + _cursorPos - srcRegion.getCentre();
					if (destRegion.containsPoint(lower)) {
						const glm::ivec3& regionUpperCorner = destRegion.getUpperCorner();
						glm::ivec3 upper = lower + srcRegion.getDimensionsInVoxels();
						if (!destRegion.containsPoint(upper)) {
							upper = regionUpperCorner;
						}
						voxel::mergeRawVolumes(_cursorPositionVolume, cropped.get(), voxel::Region(lower, upper), srcRegion);
					}
				} else {
					Log::error("Failed to crop cursor volume");
				}
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
