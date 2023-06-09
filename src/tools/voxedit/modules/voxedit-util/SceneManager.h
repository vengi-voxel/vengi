/**
 * @file
 */

#pragma once

#include "ISceneRenderer.h"
#include "MementoHandler.h"
#include "command/ActionButton.h"
#include "core/Enum.h"
#include "core/ScopedPtr.h"
#include "core/SharedPtr.h"
#include "core/Singleton.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "io/FormatDescription.h"
#include "modifier/ModifierFacade.h"
#include "modifier/ModifierType.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/Movement.h"
#include "voxedit-util/SceneRenderer.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelfont/VoxelFont.h"
#include "voxelformat/Format.h"
#include "voxelgenerator/LSystem.h"
#include "voxelgenerator/LUAGenerator.h"
#include "voxelgenerator/TreeContext.h"
#include "voxelutil/Picking.h"
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
} DIRECTIONS[] = {{"left", 1, 0, 0},  {"right", -1, 0, 0},	{"up", 0, 1, 0},
				  {"down", 0, -1, 0}, {"forward", 0, 0, 1}, {"backward", 0, 0, -1}};

enum class NodeMergeFlags {
	None = 0,
	Visible = (1 << 0),
	Locked = (1 << 1),
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
	scenegraph::SceneGraph _sceneGraph;
	MementoHandler _mementoHandler;
	util::Movement _movement;
	voxelfont::VoxelFont _voxelFont;
	core::ScopedPtr<voxel::RawVolume> _copy;
	std::future<scenegraph::SceneGraph> _loadingFuture;
	SceneRendererPtr _sceneRenderer;
	ModifierFacade _modifier;

	/**
	 * The @c video::Camera instance of the currently active @c Viewport
	 */
	video::Camera *_camera = nullptr;

	core::VarPtr _autoSaveSecondsDelay;
	core::VarPtr _gridSize;
	core::VarPtr _hideInactive;
	core::VarPtr _movementSpeed;

	math::Axis _lockedAxis = math::Axis::None;

	bool _dirty = false;
	// this is basically the same as the dirty state, but we stop
	// auto-saving once we saved a dirty state
	bool _needAutoSave = false;

	bool _traceViaMouse = true;
	int _sceneModeNodeIdTrace = InvalidNodeId;

	io::FileDescription _lastFilename;
	double _lastAutoSave = 0u;

	int _lastRaytraceX = -1;
	int _lastRaytraceY = -1;

	// model animation speed
	double _animationSpeed = 0.0;
	double _nextFrameSwitch = 0.0;
	int _currentAnimationNodeId = InvalidNodeId;

	// timeline animation
	scenegraph::FrameIndex _currentFrameIdx = 0;

	int _initialized = 0;
	int _size = 128;
	glm::ivec2 _mouseCursor{0};

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
	voxel::RawVolume *activeVolume();

	/** @return the new node id that was created from the merged nodes */
	int mergeNodes(const core::DynamicArray<int> &nodeIds);

	/**
	 * @brief Assumes that the current active scene is a fresh scene, no undo states
	 * are left, scene is no longer dirty and so on.
	 */
	void resetSceneState();
	/**
	 * @param[in] nodeId The node to set the volume for
	 * @param[in] volume The new volume - the ownership is taken over by the node if the return value of this function
	 * is @c true. If the return function is @c false, the caller has to take care about the memory of the volume.
	 * @param[in] deleteMesh TODO: handle deleteMesh somehow
	 */
	bool setNewVolume(int nodeId, voxel::RawVolume *volume, bool deleteMesh = true);
	void autosave();
	void setReferencePosition(const glm::ivec3 &pos);
	void updateGridRenderer(const voxel::Region &region);
	void zoom(video::Camera &camera, float level) const;
	bool mouseRayTrace(bool force);
	void updateCursor();
	void traceScene(bool force);

protected:
	bool setSceneGraphNodeVolume(scenegraph::SceneGraphNode &node, voxel::RawVolume *volume);
	bool loadSceneGraph(scenegraph::SceneGraph &&sceneGraph);
	int activeNode() const;

	void animate(double nowSeconds);
	/**
	 * @brief Move the cursor relative by the given steps in each direction
	 */
	void moveCursor(int x, int y, int z);
	void fillHollow();
	void hollow();

	void colorToNewNode(const voxel::Voxel voxelColor);
	void crop();
	void scaleDown(int nodeId);
	void scaleUp(int nodeId);
	void resizeAll(const glm::ivec3 &size);
	int size() const;

	bool doUndo();
	bool doRedo();

	/**
	 * @param[in] angleX in degree
	 * @param[in] angleY in degree
	 * @param[in] angleZ in degree
	 */
	void rotate(math::Axis axis);

	/**
	 * @brief Move the voxels inside the volume regions
	 */
	void move(int nodeId, const glm::ivec3 &m);

	bool saveModels(const core::String &dir);
	bool saveNode(int nodeId, const core::String &file);

	void flip(math::Axis axis);

	bool mementoStateExecute(const MementoState &s, bool isRedo);
	bool mementoStateToNode(const MementoState &s);
	bool mementoRename(const MementoState &s);
	bool mementoKeyFrames(const MementoState &s);
	bool mementoProperties(const MementoState &s);
	bool mementoPaletteChange(const MementoState &s);
	bool mementoModification(const MementoState &s);

public:
	SceneManager();
	SceneManager(const SceneRendererPtr &sceneRenderer, const ModifierRendererPtr &modifierRenderer);
	~SceneManager();

	void construct() override;
	bool init() override;
	/**
	 * @return @c true if a new scene was loaded, @c false otherwise
	 */
	bool update(double nowSeconds);
	void shutdown() override;

	void resize(int nodeId, const voxel::Region &region);
	void resize(int nodeId, const glm::ivec3 &size);

	/**
	 * @brief Shift the whole volume by the given voxel amount
	 */
	void shift(int nodeId, const glm::ivec3 &m);

	void setMousePos(int x, int y);

	bool cameraRotate() const;
	bool cameraPan() const;

	scenegraph::FrameIndex currentFrame() const;
	void setCurrentFrame(scenegraph::FrameIndex frameIdx);

	bool setAnimation(const core::String &animation);
	bool addAnimation(const core::String &animation);
	bool removeAnimation(const core::String &animation);

	void setActiveCamera(video::Camera *camera);
	video::Camera *activeCamera();

	const core::String &filename() const;
	const voxel::Voxel &hitCursorVoxel() const;

	const glm::ivec3 &cursorPosition() const;
	/**
	 * @brief Sets the cursor to the given position in the volume
	 * @note The locked axes are taken into account here and the given position might not be
	 * the final position of the cursor
	 * @param[in] force If @c true it will ignore the locked axes and still set the position
	 */
	void setCursorPosition(glm::ivec3 pos, bool force = false);

	const glm::ivec3 &referencePosition() const;

	void modified(int nodeId, const voxel::Region &modifiedRegion, bool markUndo = true,
				  uint64_t renderRegionMillis = 0);
	voxel::RawVolume *volume(int nodeId);
	const voxel::RawVolume *volume(int nodeId) const;
	voxel::Palette &activePalette() const;
	bool setActivePalette(const voxel::Palette &palette, bool searchBestColors = false);

	/**
	 * @brief Import a heightmap in the current node of the scene
	 * @param[in] file The image file to import as heightmap.
	 * @note The first component is used as height value. In most cases the R channel.
	 */
	bool importHeightmap(const core::String &file);
	bool importColoredHeightmap(const core::String &file);
	/**
	 * @brief Import an image as a plane in a new node of the scene.
	 * @note There is a total max dimension of the texture that is supported.
	 * @param[in] file The image file path to load
	 */
	bool importAsPlane(const core::String &file);
	/**
	 * @brief Import an image as a plane and apply a heightmap to it
	 */
	bool importAsVolume(const core::String &file, int maxDepth, bool bothSides);
	/**
	 * @brief Import a new palette from the given image file
	 * @note The amount of colors in the image may not exceed 256
	 * @param[in] file The image file path
	 */
	bool importPalette(const core::String &file);
	/**
	 * @param[in] paletteName The name of the palette - or a filename
	 * @note The name is extended to a filename like this @c palette-<paletteName>.[lua.png]
	 */
	bool loadPalette(const core::String &paletteName, bool searchBestColors);
	/**
	 * @brief Create a new procgen tree
	 */
	void createTree(const voxelgenerator::TreeContext &ctx);

	int addModelChild(const core::String &name, int width, int height, int depth);

	/**
	 * @brief Merge two nodes and extend the smaller one
	 */
	int mergeNodes(int nodeId1, int nodeId2);
	int mergeNodes(NodeMergeFlags flags);

	bool copy();
	bool paste(const glm::ivec3 &pos);
	bool pasteAsNewNode();
	bool cut();

	void lsystem(const core::String &axiom, const core::DynamicArray<voxelgenerator::lsystem::Rule> &rules, float angle,
				 float length, float width, float widthIncrement, int iterations, float leavesRadius);

	void fillPlane(const image::ImagePtr &image);
	void renderText(const char *text, int size = 16, int thickness = 1, int spacing = 0, const char *font = "font.ttf");

	void removeUnusedColors(int nodeId);

	/**
	 * @brief Save the volume data to the given file
	 * @param[in] file The file to store the volume data in. The file extension defines the volume format.
	 * @param[in] autosave @c true if this is an auto save action, @c false otherwise. This has e.g. an
	 * influence on the dirty state handling of the scene.
	 */
	bool save(const io::FileDescription &file, bool autosave = false);
	bool saveSelection(const io::FileDescription &file);
	/**
	 * @brief Loads a volume from the given file
	 * @param[in] file The file to load. The volume format is determined by the file extension.
	 */
	bool load(const io::FileDescription &file);
	bool load(const io::FileDescription &file, const uint8_t *data, size_t size);
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
	bool import(const core::String &file);
	bool importDirectory(const core::String &directory, const io::FormatDescription *format = nullptr, int depth = 3);

	bool runScript(const core::String &script, const core::DynamicArray<core::String> &args);

	bool newScene(bool force, const core::String &name, const voxel::Region &region);
	int addNodeToSceneGraph(scenegraph::SceneGraphNode &node, int parent = 0);

	/**
	 * @return @c true if the scene was modified and not saved yet
	 */
	bool dirty() const;
	void markDirty();

	/**
	 * @return @c true if the scene is completely empty
	 */
	bool empty() const;

	/**
	 * @note This is not about the animation scene mode, but the animation of the nodes
	 */
	bool animateActive() const;

	static const uint8_t RenderScene = 1u << 0u;
	static const uint8_t RenderUI = 1u << 1u;
	static const uint8_t RenderAll = RenderScene | RenderUI;

	/**
	 * @brief Performs the rendering for each @c Viewport instance
	 */
	void render(voxelrender::RenderContext &renderContext, const video::Camera &camera, uint8_t renderMask = RenderAll);

	/**
	 * @return @c true if the trace was executed, @c false otherwise
	 * @param[in] force Forces the trace even if the mouse did not move. This is useful for situations
	 * where the volume was modified without moving the mouse.
	 * @note The mouse trace can be disabled (might happen when you move the cursor via keyboard
	 * shortcuts). This requires the mouse to be moved before having it active again.
	 *
	 * @sa resetLastTrace()
	 */
	bool trace(bool sceneMode, bool force = false);
	void resetLastTrace();

	void updateVoxelType(int nodeId, uint8_t palIdx, voxel::VoxelType newType);

	math::Axis lockedAxis() const;
	void setLockedAxis(math::Axis axis, bool unlock);
	bool setGridResolution(int resolution);

	scenegraph::SceneGraphNode *sceneGraphNode(int nodeId);
	const scenegraph::SceneGraphNode *sceneGraphNode(int nodeId) const;

	bool hasClipboardCopy() const;

	// component access
	const ModifierFacade &modifier() const;
	ModifierFacade &modifier();
	const MementoHandler &mementoHandler() const;
	MementoHandler &mementoHandler();
	voxelgenerator::LUAGenerator &luaGenerator();
	const scenegraph::SceneGraph &sceneGraph() const;

private:
	bool isValidReferenceNode(const scenegraph::SceneGraphNode &node) const;

	void onNewNodeAdded(int newNodeId, bool isChildren = false);
	bool nodeRename(scenegraph::SceneGraphNode &node, const core::String &name);
	bool nodeRemove(scenegraph::SceneGraphNode &node, bool recursive);
	bool nodeUpdateTransform(scenegraph::SceneGraphNode &node, const glm::mat4 &localMatrix,
							 const glm::mat4 *deltaMatrix, scenegraph::KeyFrameIndex keyFrameIdx);
	bool nodeRemoveKeyFrameByIndex(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx);
	bool nodeRemoveKeyFrame(scenegraph::SceneGraphNode &node, scenegraph::FrameIndex frameIdx);
	bool nodeAddKeyframe(scenegraph::SceneGraphNode &node, scenegraph::FrameIndex frameIdx);
	void nodeDuplicate(const scenegraph::SceneGraphNode &node);
	int nodeReference(const scenegraph::SceneGraphNode &node);
	bool nodeUnreference(scenegraph::SceneGraphNode &node);

public:
	bool nodeUpdateTransform(int nodeId, const glm::mat4 &localMatrix, const glm::mat4 *deltaMatrix,
							 scenegraph::KeyFrameIndex keyFrameIdx);
	bool nodeRemoveKeyFrameByIndex(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx);
	int nodeReference(int nodeId);
	bool nodeRemoveKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx);
	bool nodeAddKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx);
	bool nodeMove(int sourceNodeId, int targetNodeId);
	bool nodeSetProperty(int nodeId, const core::String &key, const core::String &value);
	bool nodeRemoveProperty(int nodeId, const core::String &key);
	bool nodeRename(int nodeId, const core::String &name);
	bool nodeRemove(int nodeId, bool recursive);
	bool nodeSetVisible(int nodeId, bool visible);
	bool nodeSetLocked(int nodeId, bool locked);
	bool nodeActivate(int nodeId);
	bool nodeUnreference(int nodeId);
	void nodeForeachGroup(const std::function<void(int)> &f);
};

inline bool SceneManager::hasClipboardCopy() const {
	return _copy != nullptr;
}

inline scenegraph::FrameIndex SceneManager::currentFrame() const {
	return _currentFrameIdx;
}

inline void SceneManager::setCurrentFrame(scenegraph::FrameIndex frameIdx) {
	_currentFrameIdx = frameIdx;
}

inline const core::String &SceneManager::filename() const {
	return _lastFilename.name;
}

inline voxelgenerator::LUAGenerator &SceneManager::luaGenerator() {
	return _luaGenerator;
}

inline void SceneManager::setActiveCamera(video::Camera *camera) {
	if (_camera != camera) {
		resetLastTrace();
		_camera = camera;
	}
}

inline video::Camera *SceneManager::activeCamera() {
	return _camera;
}

inline math::Axis SceneManager::lockedAxis() const {
	return _lockedAxis;
}

inline const MementoHandler &SceneManager::mementoHandler() const {
	return _mementoHandler;
}

inline MementoHandler &SceneManager::mementoHandler() {
	return _mementoHandler;
}

inline bool SceneManager::dirty() const {
	return _dirty;
}

inline int SceneManager::size() const {
	return _size;
}

inline const voxel::Voxel &SceneManager::hitCursorVoxel() const {
	return _modifier.hitCursorVoxel();
}

inline const glm::ivec3 &SceneManager::cursorPosition() const {
	return _modifier.cursorPosition();
}

inline const glm::ivec3 &SceneManager::referencePosition() const {
	return _modifier.referencePosition();
}

inline const ModifierFacade &SceneManager::modifier() const {
	return _modifier;
}

inline ModifierFacade &SceneManager::modifier() {
	return _modifier;
}

inline SceneManager &sceneMgr() {
	return core::Singleton<SceneManager>::getInstance();
}

} // namespace voxedit
