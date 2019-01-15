/**
 * @file
 */

#pragma once

#include "voxel/polyvox/Picking.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/generator/LSystemGenerator.h"
#include "voxel/generator/PlantType.h"
#include "voxel/TreeContext.h"
#include "voxel/generator/BuildingGeneratorContext.h"
#include "voxel/generator/NoiseGenerator.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "render/GridRenderer.h"
#include "Action.h"
#include "voxedit-util/SelectionHandler.h"
#include "voxedit-util/ShapeHandler.h"
#include "voxedit-util/UndoHandler.h"
#include "math/Axis.h"
#include <vector>

namespace voxel {
namespace tree {
class Tree;
}
}

namespace voxedit {

static constexpr int ModelVolumeIndex = 0;
static constexpr int CursorVolumeIndex = 0;
static constexpr int SelectionVolumeIndex = 0;

/**
 * The model is shared across all viewports
 */
class Model : public core::IComponent {
private:
	voxelrender::RawVolumeRenderer _volumeRenderer;
	voxelrender::RawVolumeRenderer _cursorVolumeRenderer;
	voxelrender::RawVolumeRenderer _selectionVolumeRenderer;
	render::GridRenderer _gridRenderer;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	UndoHandler _undoHandler;
	SelectionHandler _selectionHandler;
	ShapeHandler _shapeHandler;

	glm::ivec3 _lastPlacement;
	glm::ivec3 _cursorPos;
	glm::ivec3 _referencePos;
	glm::ivec3 _mirrorPos;

	glm::ivec3 _aabbFirstPos;
	bool _aabbMode = false;

	math::Axis _lockedAxis = math::Axis::None;
	math::Axis _mirrorAxis = math::Axis::None;

	bool _empty = true;
	bool _dirty = false;

	bool _extract = false;
	bool _extractCursor = false;
	bool _extractSelection = false;

	int32_t _planeMeshIndex[3] = {-1, -1, -1};
	int32_t _mirrorMeshIndex = -1;
	int32_t _aabbMeshIndex = -1;

	int _lastRaytraceX = -1;
	int _lastRaytraceY = -1;

	int _initialized = 0;
	int _size = 32;
	int _mouseX = 0;
	int _mouseY = 0;
	uint64_t _lastActionExecution = 0;

	Action _lastAction = Action::None;
	// the action to execute on mouse move
	Action _action = Action::None;

	voxel::PickResult _result;

	uint64_t _lastGrow = 0;
	voxel::tree::Tree *_spaceColonizationTree = nullptr;

	int getIndexForAxis(math::Axis axis) const;
	int getIndexForMirrorAxis(math::Axis axis) const;
	void updateShapeBuilderForPlane(bool mirror, const glm::ivec3& pos, math::Axis axis, const glm::vec4& color);
	void markExtract();
	void markCursorExtract();
	void modified(const voxel::Region& modifiedRegion, bool markUndo = true);
	bool placeCursor(voxel::Region* modifiedRegion);
	bool setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel);
	bool actionRequiresExistingVoxel(Action action) const;
public:
	Model();
	~Model();

	void onResize(const glm::ivec2& size);

	const glm::ivec3& cursorPosition() const;
	void setCursorPosition(glm::ivec3 pos, bool force = false);

	const glm::ivec3& referencePosition() const;
	void setReferencePosition(const glm::ivec3& pos);

	bool init() override;
	void update();
	void shutdown() override;

	void copy();
	void paste();
	void cut();

	bool place();
	bool remove();

	bool aabbStart();
	bool aabbEnd();

	void crop();
	void extend(const glm::ivec3& size);
	void scaleHalf();
	void fill(const glm::ivec3& pos);
	void fill(const glm::ivec3& mins, const glm::ivec3& maxs);

	/**
	 * @brief Convert a given point cloud to voxels
	 * @param[in] vertices 3 component vertex data.
	 * @param[in] vertexColors 3 component color data
	 * @param[in] amount The amount of vertices in the buffers
	 * @note The given @c vertices coordinates must be aligned to the region of the volume already
	 * @note The color is expected to be in the range [0.0f,1.0f]
	 */
	void pointCloud(const glm::vec3* vertices, const glm::vec3 *vertexColors, size_t amount);
	bool importHeightmap(const std::string& file);
	bool save(const std::string& file);
	bool load(const std::string& file);
	bool prefab(const std::string& file);

	bool newVolume(bool force);

	const voxel::Voxel& getVoxel(const glm::ivec3& pos) const;
	bool dirty() const;
	bool needExtract() const;
	bool empty() const;
	int size() const;

	void setNewVolume(voxel::RawVolume* volume);

	void rotate(int angleX, int angleY, int angleZ);
	void move(int x, int y, int z);
	bool resample(int factor);

	void render(const video::Camera& camera);
	void renderSelection(const video::Camera& camera);

	bool renderAxis() const;
	Action evalAction() const;
	Action action() const;
	void setAction(Action action);
	Action keyAction() const;
	Action uiAction() const;

	voxel::RawVolume* modelVolume();
	const voxel::RawVolume* modelVolume() const;

	voxel::RawVolume* cursorPositionVolume();
	const voxel::RawVolume* cursorPositionVolume() const;

	voxelrender::RawVolumeRenderer& volumeRenderer();
	const voxelrender::RawVolumeRenderer& rawVolumeRenderer() const;

	render::GridRenderer& gridRenderer();

	voxelrender::RawVolumeRenderer& selectionVolumeRenderer();
	const voxelrender::RawVolumeRenderer& selectionVolumeRenderer() const;

	void spaceColonization();
	void noise(int octaves, float persistence, float lacunarity, float gain, voxel::noisegen::NoiseType type);
	void lsystem(const voxel::lsystem::LSystemContext& lsystemCtx);
	void createTree(voxel::TreeContext ctx);
	void createBuilding(voxel::BuildingType type, const voxel::BuildingContext& ctx);
	void createPlant(voxel::PlantType type);
	void createCloud();
	void createCactus();
	void bezier(const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3& control);

	bool extractVolume();
	bool extractCursorVolume();
	bool extractSelectionVolume();

	void setMousePos(int x, int y);

	bool trace(const video::Camera& camera);
	void select(const glm::ivec3& pos);
	void unselectAll();
	void executeAction(uint64_t now, bool start);
	void resetLastTrace();

	void setSelectionType(SelectType type);
	SelectType selectionType() const;

	math::Axis lockedAxis() const;
	void setLockedAxis(math::Axis axis, bool unlock);
	void updateLockedPlane(math::Axis axis);

	math::Axis mirrorAxis() const;
	void setMirrorAxis(math::Axis axis, const glm::ivec3& mirrorPos);
	void updateMirrorPlane();

	void undo();
	void redo();

	UndoHandler& undoHandler();
	const UndoHandler& undoHandler() const;

	void scaleCursorShape(const glm::vec3& scale);
	void setCursorShape(Shape shape);
	void setCursorVoxel(const voxel::Voxel& voxel);
	ShapeHandler& shapeHandler();
	const ShapeHandler& shapeHandler() const;

public:
	// TODO: maybe move into scene
	bool _renderAxis = true;
	bool _renderLockAxis = true;
	// the key action - has a higher priority than the ui action
	Action _keyAction = Action::None;
	// action that is selected via ui
	Action _uiAction = Action::PlaceVoxels;
	uint64_t _actionExecutionDelay = 20;
};

inline math::Axis Model::lockedAxis() const {
	return _lockedAxis;
}

inline void Model::scaleCursorShape(const glm::vec3& scale) {
	_shapeHandler.scaleCursorShape(scale, cursorPositionVolume());
	resetLastTrace();
	markCursorExtract();
}

inline void Model::setCursorVoxel(const voxel::Voxel& type) {
	_shapeHandler.setCursorVoxel(type);
	voxel::RawVolume* cursorVolume = cursorPositionVolume();
	if (cursorVolume != nullptr) {
		_shapeHandler.setCursorShape(_shapeHandler.cursorShape(), cursorVolume, true);
	}
	markCursorExtract();
}

inline void Model::setCursorShape(Shape shape) {
	_shapeHandler.setCursorShape(shape, cursorPositionVolume(), true);
	resetLastTrace();
	markCursorExtract();
}

inline ShapeHandler& Model::shapeHandler() {
	return _shapeHandler;
}

inline const ShapeHandler& Model::shapeHandler() const {
	return _shapeHandler;
}

inline UndoHandler& Model::undoHandler() {
	return _undoHandler;
}

inline const UndoHandler& Model::undoHandler() const {
	return _undoHandler;
}

inline void Model::setSelectionType(SelectType type) {
	_selectionHandler.setSelectionType(type);
}

inline SelectType Model::selectionType() const {
	return _selectionHandler.selectionType();
}

inline bool Model::renderAxis() const {
	return _renderAxis;
}

inline Action Model::evalAction() const {
	if (_keyAction != Action::None) {
		return _keyAction;
	}
	if (action() != Action::None) {
		return action();
	}
	return _uiAction;
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
	return _volumeRenderer.volume(ModelVolumeIndex);
}

inline const voxel::RawVolume* Model::modelVolume() const {
	return _volumeRenderer.volume(ModelVolumeIndex);
}

inline voxel::RawVolume* Model::cursorPositionVolume() {
	return _cursorVolumeRenderer.volume(CursorVolumeIndex);
}

inline const voxel::RawVolume* Model::cursorPositionVolume() const {
	return _cursorVolumeRenderer.volume(CursorVolumeIndex);
}

inline Action Model::uiAction() const {
	return _uiAction;
}

inline voxelrender::RawVolumeRenderer& Model::volumeRenderer() {
	return _volumeRenderer;
}

inline const voxelrender::RawVolumeRenderer& Model::rawVolumeRenderer() const {
	return _volumeRenderer;
}

inline render::GridRenderer& Model::gridRenderer() {
	return _gridRenderer;
}

inline voxelrender::RawVolumeRenderer& Model::selectionVolumeRenderer() {
	return _selectionVolumeRenderer;
}

inline const voxelrender::RawVolumeRenderer& Model::selectionVolumeRenderer() const {
	return _selectionVolumeRenderer;
}

inline bool Model::actionRequiresExistingVoxel(Action action) const {
	return action == Action::CopyVoxel || action == Action::DeleteVoxel
			|| action == Action::OverrideVoxel || action == Action::SelectVoxels;
}

inline bool Model::dirty() const {
	return _dirty;
}

inline bool Model::needExtract() const {
	return _extract;
}

inline int Model::size() const {
	return _size;
}

inline bool Model::empty() const {
	return _empty;
}

inline const glm::ivec3& Model::cursorPosition() const {
	return _cursorPos;
}

inline const glm::ivec3& Model::referencePosition() const {
	return _referencePos;
}

}
