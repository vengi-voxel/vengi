/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "core/collection/DynamicArray.h"
#include "voxelfont/VoxelFont.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelutil/Picking.h"
#include "voxel/RawVolume.h"
#include "voxelgenerator/TreeContext.h"
#include "voxelgenerator/LSystem.h"
#include "voxelrender/SceneGraphRenderer.h"
#include "voxelformat/Format.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "render/GridRenderer.h"
#include "core/Var.h"
#include "core/Singleton.h"
#include "command/ActionButton.h"
#include "math/Axis.h"
#include "math/OBB.h"
#include "MementoHandler.h"
#include "voxelgenerator/LUAGenerator.h"
#include "modifier/ModifierType.h"
#include "modifier/ModifierFacade.h"
#include <functional>

namespace voxedit {

/**
 * @brief Move directions for the cursor
 */
static constexpr struct Direction {
	const char *postfix;
	int x;
	int y;
	int z;
} DIRECTIONS[] = {
	{"left",      1,  0,  0},
	{"right",    -1,  0,  0},
	{"up",        0,  1,  0},
	{"down",      0, -1,  0},
	{"forward",   0,  0,  1},
	{"backward",  0,  0, -1}
};

enum class EditMode {
	// Edit a model volume (voxels)
	Model,
	// Edit the scene (volume positions, rotations, ...)
	Scene
};

enum class NodeMergeFlags {
	None      = 0,
	Visible   = (1 << 0),
	Locked    = (1 << 1),
	Invisible = (1 << 2),
	Max,
	All = Visible | Locked | Invisible
};
CORE_ENUM_BIT_OPERATIONS(NodeMergeFlags)

/**
 * @note The data is shared across all viewports
 */
class SceneManager : public core::IComponent {
private:
	voxelformat::SceneGraph _sceneGraph;
	voxelrender::SceneGraphRenderer _volumeRenderer;
	render::GridRenderer _gridRenderer;
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	MementoHandler _mementoHandler;
	ModifierFacade _modifier;
	voxelfont::VoxelFont _voxelFont;
	voxel::RawVolume* _copy = nullptr;
	EditMode _editMode = EditMode::Scene;
	std::future<voxelformat::SceneGraph> _loadingFuture;

	/**
	 * The @c video::Camera instance of the currently active @c Viewport
	 */
	video::Camera* _camera = nullptr;

	int32_t _aabbMeshIndex = -1;
	int32_t _referencePointMesh = -1;
	glm::mat4 _referencePointModelMatrix { 1.0f };

	core::VarPtr _autoSaveSecondsDelay;
	core::VarPtr _ambientColor;
	core::VarPtr _diffuseColor;
	core::VarPtr _grayInactive;
	core::VarPtr _hideInactive;
	core::VarPtr _showAabbVar;

	struct DirtyRegion {
		voxel::Region region;
		int nodeId;
	};
	using RegionQueue = core::DynamicArray<DirtyRegion>;
	RegionQueue _extractRegions;
	void queueRegionExtraction(int layerId, const voxel::Region& region);

	math::Axis _lockedAxis = math::Axis::None;

	bool _dirty = false;
	// this is basically the same as the dirty state, but we stop
	// auto-saving once we saved a dirty state
	bool _needAutoSave = false;

	bool _renderShadow = true;
	bool _renderLockAxis = true;

	bool _traceViaMouse = true;
	int _sceneModeNodeIdTrace = -1;

	core::String _lastFilename;
	double _lastAutoSave = 0u;

	int32_t _planeMeshIndex[3] = {-1, -1, -1};

	int _lastRaytraceX = -1;
	int _lastRaytraceY = -1;

	// layer animation speed
	int _currentAnimationLayer = 0;
	double _animationSpeed = 0.0;
	double _nextFrameSwitch = 0.0;
	voxelformat::FrameIndex _currentFrameIdx = 0;

	int _initialized = 0;
	int _size = 128;
	glm::ivec2 _mouseCursor { 0 };

	command::ActionButton _move[lengthof(DIRECTIONS)];
	command::ActionButton _rotate;
	command::ActionButton _pan;
	command::ActionButton _zoomIn;
	command::ActionButton _zoomOut;

	voxelutil::PickResult _result;

	voxelgenerator::LUAGenerator _luaGenerator;

	/**
	 * @note This might return @c nullptr in the case where the active node is no model node
	 */
	voxel::RawVolume* activeVolume();

	/** @return the new node id that was created from the merged nodes */
	int mergeNodes(const core::DynamicArray<int>& nodeIds);

	/**
	 * @brief Assumes that the current active scene is a fresh scene, no undo states
	 * are left, scene is no longer dirty and so on.
	 */
	void resetSceneState();
	bool setNewVolume(int nodeId, voxel::RawVolume* volume, bool deleteMesh = true);
	void autosave();
	void setReferencePosition(const glm::ivec3& pos);
	void updateGridRenderer(const voxel::Region& region);
	void zoom(video::Camera& camera, float level) const;
	bool extractVolume();
	void updateLockedPlane(math::Axis axis);
	void updateAABBMesh();
	math::AABB<float> toAABB(const voxel::Region& region) const;
	math::OBB<float> toOBB(const voxel::Region& region, const voxelformat::SceneGraphTransform &transform) const;
protected:
	voxelformat::SceneGraphNode *sceneGraphNode(int nodeId);
	const voxelformat::SceneGraphNode *sceneGraphNode(int nodeId) const;
	bool setSceneGraphNodeVolume(voxelformat::SceneGraphNode &node, voxel::RawVolume* volume);
	bool loadSceneGraph(voxelformat::SceneGraph&& sceneGraph);
	int activeNode() const;
	int addModelChild(const core::String& name, int width, int height, int depth);

	void animate(double nowSeconds);
	/**
	 * @brief Move the cursor relative by the given steps in each direction
	 */
	void moveCursor(int x, int y, int z);
	void fillHollow();

	void colorToNewLayer(const voxel::Voxel voxelColor);
	void crop();
	void scale(int nodeId);
	void resizeAll(const glm::ivec3& size);
	int size() const;

	/**
	 * @brief Merge two nodes and extend the smaller one
	 */
	int mergeNodes(int nodeId1, int nodeId2);
	int mergeNodes(NodeMergeFlags flags);

	bool doUndo();
	bool doRedo();

	bool copy();
	bool paste(const glm::ivec3& pos);
	bool cut();

	void rotate(int angleX, int angleY, int angleZ);

	/**
	 * @brief Move the voxels inside the volume regions
	 */
	void move(int nodeId, const glm::ivec3& m);

	bool saveModels(const core::String& dir);
	bool saveNode(int nodeId, const core::String& file);

	void flip(math::Axis axis);

	bool mementoRename(const MementoState& s);
	bool mementoPaletteChange(const MementoState& s);
	bool mementoModification(const MementoState& s);

public:
	~SceneManager();

	void construct() override;
	bool init() override;
	bool update(double nowSeconds);
	void shutdown() override;

	void resize(int nodeId, const voxel::Region &region);
	void resize(int nodeId, const glm::ivec3& size);

	/**
	 * @brief Shift the whole volume by the given voxel amount
	 */
	void shift(int nodeId, const glm::ivec3& m);

	void setMousePos(int x, int y);

	bool cameraRotate() const;
	bool cameraPan() const;

	voxelformat::FrameIndex currentFrame() const;
	void setCurrentFrame(voxelformat::FrameIndex frameIdx);

	void setActiveCamera(video::Camera* camera);
	video::Camera* activeCamera();

	const core::String& filename() const;
	const voxel::Voxel& hitCursorVoxel() const;

	const glm::ivec3& cursorPosition() const;
	/**
	 * @brief Sets the cursor to the given position in the volume
	 * @note The locked axes are taken into account here and the given position might not be
	 * the final position of the cursor
	 * @param[in] force If @c true it will ignore the locked axes and still set the position
	 */
	void setCursorPosition(glm::ivec3 pos, bool force = false);

	const glm::ivec3& referencePosition() const;

	void modified(int nodeId, const voxel::Region& modifiedRegion, bool markUndo = true);
	voxel::RawVolume* volume(int nodeId);
	const voxel::RawVolume* volume(int nodeId) const;
	voxel::Palette &activePalette() const;
	bool setActivePalette(const voxel::Palette &palette, bool searchBestColors = false);

	/**
	 * @brief Import a heightmap in the current layer of the scene
	 * @param[in] file The image file to import as heightmap.
	 * @note The first component is used as height value. In most cases the R channel.
	 */
	bool importHeightmap(const core::String& file);
	bool importColoredHeightmap(const core::String& file);
	/**
	 * @brief Import an image as a plane in a new layer of the scene.
	 * @note There is a total max dimension of the texture that is supported.
	 * @param[in] file The image file path to load
	 */
	bool importAsPlane(const core::String& file);
	/**
	 * @brief Import an image as a plane and apply a heightmap to it
	 */
	bool importAsVolume(const core::String &file, int maxDepth, bool bothSides);
	/**
	 * @brief Import a new palette from the given image file
	 * @note The amount of colors in the image may not exceed 256
	 * @param[in] file The image file path
	 */
	bool importPalette(const core::String& file);
	/**
	 * @param[in] paletteName The name of the palette
	 * @note The name is extended to a filename like this @c palette-<paletteName>.[lua.png]
	 */
	bool loadPalette(const core::String& paletteName, bool searchBestColors = false);
	/**
	 * @brief Create a new procgen tree
	 */
	void createTree(const voxelgenerator::TreeContext& ctx);

	void lsystem(const core::String &axiom, const core::DynamicArray<voxelgenerator::lsystem::Rule> &rules, float angle,
			float length, float width, float widthIncrement, int iterations, float leavesRadius);

	void fillPlane(const image::ImagePtr &image);
	void renderText(const char *text, int size = 16, int thickness = 1, int spacing = 0, const char *font = "font.ttf");

	/**
	 * @brief Save the volume data to the given file
	 * @param[in] file The file to store the volume data in. The file extension defines the volume format.
	 * @param[in] autosave @c true if this is an auto save action, @c false otherwise. This has e.g. an
	 * influence on the dirty state handling of the scene.
	 */
	bool save(const core::String& file, bool autosave = false);
	/**
	 * @brief Loads a volume from the given file
	 * @param[in] file The file to load. The volume format is determined by the file extension.
	 */
	bool load(const core::String& file);
	bool isLoading() const;

	bool undo(int n = 1);
	bool redo(int n = 1);

	/**
	 * @brief Shift the whole volume by the given voxel amount
	 */
	void shift(int x, int y, int z);
	/**
	 * @brief Move the voxels inside the volume regions
	 */
	void move(int x, int y, int z);

	/**
	 * @brief Import an existing model
	 * @note Placed relative to the reference position in the current scene
	 */
	bool prefab(const core::String& file);

	bool runScript(const core::String& script, const core::DynamicArray<core::String>& args);

	bool newScene(bool force, const core::String& name, const voxel::Region& region);
	int addNodeToSceneGraph(voxelformat::SceneGraphNode &node, int parent = 0);

	/**
	 * @return @c true if the scene was modified and not saved yet
	 */
	bool dirty() const;
	void markDirty();

	/**
	 * @return @c true if the scene is completely empty
	 */
	bool empty() const;
	EditMode editMode() const;

	/**
	 * @note This is not about the animation scene mode, but the animation of the layers
	 */
	bool animateActive() const;

	static const uint8_t RenderScene = 1u << 0u;
	static const uint8_t RenderUI    = 1u << 1u;
	static const uint8_t RenderAll   = RenderScene | RenderUI;

	/**
	 * @brief Performs the rendering for each @c Viewport instance
	 */
	void render(voxelrender::RenderContext &renderContext, const video::Camera& camera, uint8_t renderMask = RenderAll);

	/**
	 * @return @c true if the trace was executed, @c false otherwise
	 * @param[in] force Forces the trace even if the mouse did not move. This is useful for situations
	 * where the volume was modified without moving the mouse.
	 * @note The mouse trace can be disabled (might happen when you move the cursor via keyboard
	 * shortcuts). This requires the mouse to be moved before having it active again.
	 *
	 * @sa resetLastTrace()
	 */
	bool trace(bool force = false, voxelutil::PickResult *result = nullptr);
	void resetLastTrace();

	math::Axis lockedAxis() const;
	void setLockedAxis(math::Axis axis, bool unlock);
	void setRenderLockAxis(bool renderLockAxis);
	void setRenderShadow(bool shadow);
	bool setGridResolution(int resolution);

	// component access
	const ModifierFacade& modifier() const;
	ModifierFacade& modifier();
	const MementoHandler& mementoHandler() const;
	MementoHandler& mementoHandler();
	const voxelrender::SceneGraphRenderer& renderer() const;
	voxelrender::SceneGraphRenderer& renderer();
	render::GridRenderer& gridRenderer();
	voxelgenerator::LUAGenerator& luaGenerator();
	video::ShapeBuilder& shapeBuilder();
	render::ShapeRenderer& shapeRenderer();
	const voxelformat::SceneGraph &sceneGraph();
	void setEditMode(EditMode mode);
	void toggleEditMode();

	bool hasClipboardCopy() const;

private:
	void onNewNodeAdded(int newNodeId);
	bool nodeRename(voxelformat::SceneGraphNode &node, const core::String &name);
	bool nodeRemove(voxelformat::SceneGraphNode &node, bool recursive);
	bool nodeUpdateTransform(voxelformat::SceneGraphNode &node, const glm::mat4 &localMatrix, const glm::mat4 *deltaMatrix, voxelformat::KeyFrameIndex keyFrameIdx);
	void nodeDuplicate(const voxelformat::SceneGraphNode &node);
public:
	bool nodeUpdateTransform(int nodeId, const glm::mat4 &localMatrix, const glm::mat4 *deltaMatrix, voxelformat::KeyFrameIndex keyFrameIdx);
	bool nodeMove(int sourceNodeId, int targetNodeId);
	bool nodeRename(int nodeId, const core::String &name);
	bool nodeRemove(int nodeId, bool recursive);
	bool nodeSetVisible(int nodeId, bool visible);
	bool nodeSetLocked(int nodeId, bool visible);
	bool nodeActivate(int nodeId);
	void nodeForeachGroup(const std::function<void(int)>& f);
};

inline bool SceneManager::hasClipboardCopy() const {
	return _copy != nullptr;
}

inline voxelformat::FrameIndex SceneManager::currentFrame() const {
	return _currentFrameIdx;
}

inline void SceneManager::setCurrentFrame(voxelformat::FrameIndex frameIdx) {
	_currentFrameIdx = frameIdx;
}

inline const core::String& SceneManager::filename() const {
	return _lastFilename;
}

inline video::ShapeBuilder &SceneManager::shapeBuilder() {
	return _shapeBuilder;
}

inline render::ShapeRenderer &SceneManager::shapeRenderer() {
	return _shapeRenderer;
}

inline voxelgenerator::LUAGenerator& SceneManager::luaGenerator() {
	return _luaGenerator;
}

inline void SceneManager::setRenderLockAxis(bool renderLockAxis) {
	_renderLockAxis = renderLockAxis;
}

inline void SceneManager::setRenderShadow(bool shadow) {
	_renderShadow = shadow;
}

inline void SceneManager::setActiveCamera(video::Camera* camera) {
	if (_camera != camera) {
		resetLastTrace();
		_camera = camera;
	}
}

inline video::Camera* SceneManager::activeCamera() {
	return _camera;
}

inline voxelrender::SceneGraphRenderer& SceneManager::renderer() {
	return _volumeRenderer;
}

inline const voxelrender::SceneGraphRenderer& SceneManager::renderer() const {
	return _volumeRenderer;
}

inline math::Axis SceneManager::lockedAxis() const {
	return _lockedAxis;
}

inline const MementoHandler& SceneManager::mementoHandler() const {
	return _mementoHandler;
}

inline MementoHandler& SceneManager::mementoHandler() {
	return _mementoHandler;
}

inline render::GridRenderer& SceneManager::gridRenderer() {
	return _gridRenderer;
}

inline bool SceneManager::dirty() const {
	return _dirty;
}

inline int SceneManager::size() const {
	return _size;
}

inline EditMode SceneManager::editMode() const {
	return _editMode;
}

inline const voxel::Voxel& SceneManager::hitCursorVoxel() const {
	return _modifier.hitCursorVoxel();
}

inline const glm::ivec3& SceneManager::cursorPosition() const {
	return _modifier.cursorPosition();
}

inline const glm::ivec3& SceneManager::referencePosition() const {
	return _modifier.referencePosition();
}

inline const ModifierFacade& SceneManager::modifier() const {
	return _modifier;
}

inline ModifierFacade& SceneManager::modifier() {
	return _modifier;
}

inline SceneManager& sceneMgr() {
	return core::Singleton<SceneManager>::getInstance();
}

}
