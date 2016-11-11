#pragma once

#include "voxel/polyvox/Picking.h"
#include "voxel/polyvox/RawVolume.h"
#include "frontend/RawVolumeRenderer.h"
#include "Action.h"
#include "SelectType.h"
#include "Shape.h"
#include "Axis.h"
#include <vector>

namespace voxedit {

/**
 * The model is shared across all viewports
 */
class Model {
private:
	enum class CursorShapeState {
		/** just switched the shape type */
		New,
		/** clicked into the volume but didn't yet configure the cursor shape - e.g. modify the circle size */
		Configure,
		/** fully created the cursor volume, can be placed on click */
		Created
	};

	std::vector<voxel::RawVolume*> _undoStates;
	uint8_t _undoIndex = 0u;
	static constexpr int _maxUndoStates = 64;

	int _initialized = 0;
	voxel::Voxel _currentVoxel;
	int _size = 32;
	frontend::RawVolumeRenderer _rawVolumeRenderer;
	frontend::RawVolumeRenderer _rawVolumeSelectionRenderer;

	int _mouseX = 0;
	int _mouseY = 0;

	SelectType _selectionType = SelectType::Single;
	Shape _cursorShape = Shape::Single;
	CursorShapeState _cursorShapeState = CursorShapeState::New;
	glm::ivec3 _lastPlacement;
	Axis _lockedAxis = Axis::None;
	glm::ivec3 _cursorPos;

	bool actionRequiresExistingVoxel(Action action) const;
	void clearUndoStates();
	void markUndo();
public:
	Model();
	~Model();

	void onResize(const glm::ivec2& size);

	const voxel::Voxel& currentVoxel() const;

	void init();
	void shutdown();

	void copy();
	void paste();
	void cut();

	void crop();
	void extend(int size = 1);

	void undo();
	void redo();
	bool canUndo() const;
	bool canRedo() const;

	bool save(std::string_view file);
	bool load(std::string_view file);

	bool newVolume(bool force);

	const voxel::Voxel& getVoxel(const glm::ivec3& pos) const;
	bool setVoxel(glm::ivec3 pos, const voxel::Voxel& voxel);
	bool dirty() const;
	bool empty() const;
	float size() const;

	void setNewVolume(voxel::RawVolume* volume);

	void rotate(int angleX, int angleY, int angleZ);

	void render(const video::Camera& camera);
	void renderSelection(const video::Camera& camera);

	bool renderAxis() const;
	Action action() const;
	void setAction(Action action);
	Action keyAction() const;
	Action uiAction() const;

	void setVoxelType(voxel::VoxelType type);
	voxel::RawVolume* modelVolume();
	const voxel::RawVolume* modelVolume() const;

	frontend::RawVolumeRenderer& rawVolumeRenderer();
	const frontend::RawVolumeRenderer& rawVolumeRenderer() const;

	frontend::RawVolumeRenderer& rawVolumeSelectionRenderer();
	const frontend::RawVolumeRenderer& rawVolumeSelectionRenderer() const;

	voxel::PickResult& result();

	bool extractVolume();
	bool extractSelectionVolume();

	void setMousePos(int x, int y);

	bool trace(bool skipCursor, const video::Camera& camera);
	void select(const glm::ivec3& pos);
	void unselectAll();
	void executeAction(bool mouseDown, long now);
	void resetLastTrace();

	void setSelectionType(SelectType type);
	SelectType selectionType() const;

	Shape cursorShape() const;
	bool setCursorShape(Shape type, bool force = false);

	Axis lockedAxis() const;
	void setLockedAxis(Axis axis, bool unlock);

public:
	// TODO: maybe move into scene
	bool _renderAxis = true;
	bool _dirty = false;
	bool _extract = false;
	bool _empty = true;
	bool _selectionExtract = false;
	int _lastRaytraceX = -1;
	int _lastRaytraceY = -1;
	long _actionExecutionDelay = 5l;
	long _lastActionExecution = 0l;
	Action _lastAction = Action::None;
	// the action to execute on mouse move
	Action _action = Action::None;
	// the key action - has a higher priority than the ui action
	Action _keyAction = Action::None;
	// action that is selected via ui
	Action _uiAction = Action::PlaceVoxel;
	voxel::PickResult _result;

	// the shape of the cursor at the center of the volume
	voxel::RawVolume* _cursorVolume = nullptr;
	// the cursor shape at the position of the traced voxel - same size as the model volume
	voxel::RawVolume* _cursorPositionVolume = nullptr;
	voxel::RawVolume* _modelVolume = nullptr;
};

inline bool Model::canUndo() const {
	return _undoIndex > 0;
}

inline bool Model::canRedo() const {
	if (_undoStates.empty()) {
		return false;
	}
	return _undoIndex < _undoStates.size() - 1;
}

inline Shape Model::cursorShape() const {
	return _cursorShape;
}

inline Axis Model::lockedAxis() const {
	return _lockedAxis;
}

inline void Model::setLockedAxis(Axis axis, bool unlock) {
	if (unlock) {
		_lockedAxis &= ~axis;
	} else {
		_lockedAxis |= axis;
	}
}

inline void Model::setSelectionType(SelectType type) {
	_selectionType = type;
}

inline SelectType Model::selectionType() const {
	return _selectionType;
}

inline void Model::setVoxelType(voxel::VoxelType type) {
	Log::info("Change voxel to %i", std::enum_value(type));
	_currentVoxel = voxel::createVoxel(type);
}

inline const voxel::Voxel& Model::currentVoxel() const {
	return _currentVoxel;
}

inline voxel::PickResult& Model::result() {
	return _result;
}

inline bool Model::renderAxis() const {
	return _renderAxis;
}

inline Action Model::action() const {
	return _action;
}

inline void Model::setAction(Action action) {
	_action = action;
}

inline Action Model::keyAction() const {
	return _keyAction;
}

inline voxel::RawVolume* Model::modelVolume() {
	return _modelVolume;
}

inline const voxel::RawVolume* Model::modelVolume() const {
	return _modelVolume;
}

inline Action Model::uiAction() const {
	return _uiAction;
}

inline frontend::RawVolumeRenderer& Model::rawVolumeRenderer() {
	return _rawVolumeRenderer;
}

inline const frontend::RawVolumeRenderer& Model::rawVolumeRenderer() const {
	return _rawVolumeRenderer;
}

inline frontend::RawVolumeRenderer& Model::rawVolumeSelectionRenderer() {
	return _rawVolumeSelectionRenderer;
}

inline const frontend::RawVolumeRenderer& Model::rawVolumeSelectionRenderer() const {
	return _rawVolumeSelectionRenderer;
}

inline bool Model::actionRequiresExistingVoxel(Action action) const {
	return action == Action::CopyVoxel || action == Action::DeleteVoxel
			|| action == Action::OverrideVoxel || action == Action::SelectVoxels;
}

inline bool Model::dirty() const {
	return _dirty;
}

inline float Model::size() const {
	return _size;
}

inline bool Model::empty() const {
	return _empty;
}

}
