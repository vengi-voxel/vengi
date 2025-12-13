/**
 * @file
 */

#pragma once

#include "Clipboard.h"
#include "ISceneRenderer.h"
#include "LUAApiListener.h"
#include "command/ActionButton.h"
#include "core/DeltaFrameSeconds.h"
#include "core/Enum.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Future.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "memento/MementoHandler.h"
#include "modifier/IModifierRenderer.h"
#include "modifier/ModifierFacade.h"
#include "modifier/SceneModifiedFlags.h"
#include "modifier/SelectionManager.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "voxedit-util/network/Client.h"
#include "voxedit-util/network/Server.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/Format.h"
#include "voxelgenerator/LSystem.h"
#include "voxelgenerator/LUAApi.h"
#include "voxelgenerator/TreeContext.h"
#include "voxelrender/CameraMovement.h"
#include "voxelrender/RawVolumeRenderer.h"
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
class SceneManager : public core::DeltaFrameSeconds {
	friend class LUAApiListener;

protected:
	scenegraph::SceneGraph _sceneGraph;
	voxelrender::CameraMovement _camMovement;
	memento::MementoHandler _mementoHandler;
	voxel::ClipboardData _copy;
	core::Future<scenegraph::SceneGraph> _loadingFuture;
	core::TimeProviderPtr _timeProvider;
	SceneRendererPtr _sceneRenderer;
	ModifierFacade _modifierFacade;
	voxelgenerator::LUAApi _luaApi;
	LUAApiListener _luaApiListener;
	io::FilesystemPtr _filesystem;
	SelectionManagerPtr _selectionManager;
	Server _server;
	Client _client;

	/**
	 * The @c video::Camera instance of the currently active @c Viewport
	 */
	video::Camera *_camera = nullptr;

	core::VarPtr _autoSaveSecondsDelay;
	core::VarPtr _gridSize;
	core::VarPtr _transformUpdateChildren;
	core::VarPtr _maxSuggestedVolumeSize;
	core::VarPtr _lastDirectory;

	bool _dirty = false;
	// this is basically the same as the dirty state, but we stop
	// auto-saving once we saved a dirty state
	bool _needAutoSave = false;
	bool _traceViaMouse = true;

	io::FileDescription _lastFilename;
	double _lastAutoSave = 0u;

	int _lastRaytraceX = -1;
	int _lastRaytraceY = -1;

	static const uint32_t DirtyRendererLockedAxis = 1 << 0;
	static const uint32_t DirtyRendererGridRenderer = 1 << 1;
	uint32_t _dirtyRenderer = 0u;

	// model animation speed
	double _animationSpeed = 0.0;
	double _nextFrameSwitch = 0.0;
	int _currentAnimationNodeId = InvalidNodeId;
	bool _animationResetCamera = false;

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

	/**
	 * @note This might return @c nullptr in the case where the active node is no model node
	 */
	voxel::RawVolume *activeVolume();

	/** @return the new node id that was created from the merged nodes */
	int mergeNodes(const core::Buffer<int> &nodeIds);

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
	void updateDirtyRendererStates();
	void zoom(video::Camera &camera, float level);
	bool mouseRayTrace(bool force, const glm::mat4 &invModel);
	void updateCursor();
	int traceScene();

protected:
	bool setSceneGraphNodeVolume(scenegraph::SceneGraphNode &node, voxel::RawVolume *volume);
	int activeNode() const;

	void animate(double nowSeconds);
	/**
	 * @brief Move the cursor relative by the given steps in each direction
	 */
	void moveCursor(int x, int y, int z);
	void fillHollow();
	void hollow();
	void fill();
	void clear();

	int colorToNewNode(int nodeId, const voxel::Voxel voxelColor);
	int colorToNewNode(const voxel::Voxel voxelColor);
	void crop();
	void splitObjects();
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

	bool saveModels(const core::String &dir);
	bool saveNode(int nodeId, const core::String &file);

	void flip(math::Axis axis);

	/**
	 * we assume that this is going hand in hand with transform states
	 * see @c MementoType::SceneGraphAnimation resp. @c MementoHandler::markAddedAnimation() and @c
	 * MementoHandler::markRemovedAnimation()
	 */
	bool mementoAnimations(const memento::MementoState &s);
	bool mementoStateExecute(const memento::MementoState &s, bool isRedo);
	bool mementoStateToNode(const memento::MementoState &s);
	bool mementoRename(const memento::MementoState &s);
	bool mementoKeyFrames(const memento::MementoState &s);
	bool mementoProperties(const memento::MementoState &s);
	bool mementoPaletteChange(const memento::MementoState &s);
	bool mementoNormalPaletteChange(const memento::MementoState &s);
	bool mementoModification(const memento::MementoState &s);

	/**
	 * @brief Sets the cursor to the given position in the volume
	 * @note The locked axes are taken into account here and the given position might not be
	 * the final position of the cursor
	 * @param[in] force If @c true it will ignore the locked axes and still set the position
	 */
	void setCursorPosition(glm::ivec3 pos, voxel::FaceNames hitFace, bool force = false);

public:
	SceneManager(const core::TimeProviderPtr &timeProvider, const io::FilesystemPtr &filesystem,
				 const SceneRendererPtr &sceneRenderer, const ModifierRendererPtr &modifierRenderer,
				 const SelectionManagerPtr &selectionManager);
	~SceneManager();

	void construct() override;
	bool init() override;
	/**
	 * @return @c true if a new scene was loaded, @c false otherwise
	 */
	bool update(double nowSeconds);
	void shutdown() override;

	/**
	 * @brief Update the cursor position used for tracing
	 */
	void setMousePos(int x, int y);

	/**
	 * @return @c true if the current node supports switching to edit mode
	 */
	bool supportsEditMode() const;

	/**
	 * @brief world matrix for the current active node
	 */
	glm::mat4 worldMatrix(scenegraph::FrameIndex frameIdx = 0, bool applyTransforms = true) const;

	const voxel::Region &sliceRegion() const;
	void setSliceRegion(const voxel::Region &region);
	bool isSliceModeActive() const;

	bool exceedsMaxSuggestedVolumeSize() const;

	bool cameraRotate() const;
	bool cameraPan() const;

	scenegraph::SceneGraphNodeCamera *activeCameraNode();

	bool clipping() const;
	void setClipping(bool enabled);

	bool gravity() const;
	void setGravity(bool enabled);

	scenegraph::FrameIndex currentFrame() const;
	void setCurrentFrame(scenegraph::FrameIndex frameIdx);

	bool setAnimation(const core::String &animation);
	bool addAnimation(const core::String &animation);
	bool duplicateAnimation(const core::String &animation, const core::String &newName);
	bool removeAnimation(const core::String &animation);

	void setActiveCamera(video::Camera *camera);
	video::Camera *activeCamera() const;

	core::String getSuggestedFilename(const core::String &extension = "") const;
	const voxel::Voxel &hitCursorVoxel() const;

	/**
	 * @brief The cursor position is in model space - it's a coordinate in the volume, not taking the transform into
	 * account (but the region)
	 */
	const glm::ivec3 &cursorPosition() const;

	/**
	 * @brief The reference position is in model space - it's a coordinate in the volume, not taking the transform into
	 * account (but the region)
	 */
	const glm::ivec3 &referencePosition() const;

	void modified(int nodeId, const voxel::Region &modifiedRegion, SceneModifiedFlags flags = SceneModifiedFlags::All,
				  uint64_t renderRegionMillis = 0);
	voxel::RawVolume *volume(int nodeId);
	const voxel::RawVolume *volume(int nodeId) const;
	palette::Palette &activePalette() const;

	bool setActivePalette(const palette::Palette &palette, bool searchBestColors = false);

	/**
	 * @brief Import a new palette from the given image file
	 * @note The amount of colors in the image may not exceed 256
	 * @param[in] file The image file path
	 */
	bool importPalette(const core::String &file, bool setActive, bool searchBestColors);
	/**
	 * @param[in] paletteName The name of the palette - or a filename
	 * @note The name is extended to a filename like this @c palette-<paletteName>.[lua.png]
	 */
	bool loadPalette(const core::String &paletteName, bool searchBestColors, bool save);

	bool calculateNormals(int nodeId, voxel::Connectivity connectivity, bool recalcAll = false,
						  bool fillAndHollow = false);

	/**
	 * @brief Create a new procgen tree
	 */
	void createTree(const voxelgenerator::TreeContext &ctx);

	/**
	 * @brief Add a new model node as children to the current active node
	 */
	int addModelChild(const core::String &name, int width, int height, int depth, const core::UUID &uuid = core::UUID());
	int addPointChild(const core::String &name, const glm::ivec3 &position, const glm::quat &orientation,
					  const core::UUID &uuid = core::UUID());

	/**
	 * @brief Merge two nodes and extend the smaller one
	 */
	int mergeNodes(int nodeId1, int nodeId2);
	int mergeNodes(NodeMergeFlags flags);

	/**
	 * @brief Split volumes into smaller volumes to improve performance
	 */
	bool splitVolumes();

	bool copy();
	bool paste(const glm::ivec3 &pos);
	bool pasteAsNewNode();
	bool cut();

	void lsystem(const core::String &axiom, const core::DynamicArray<voxelgenerator::lsystem::Rule> &rules, float angle,
				 float length, float width, float widthIncrement, int iterations, float leavesRadius);

	void fillPlane(const image::ImagePtr &image);

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
	bool loadSceneGraph(scenegraph::SceneGraph &&sceneGraph, bool disconnect = true);

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

	bool isScriptRunning() const;
	bool runScript(const core::String &luaCode, const core::DynamicArray<core::String> &args);

	/**
	 * @brief Take ownership of the volume if this returns @c true, otherwise the caller must free the memory
	 */
	bool newScene(bool force, const core::String &name, voxel::RawVolume *v);
	bool newScene(bool force, const core::String &name, const voxel::Region &region);
	int moveNodeToSceneGraph(scenegraph::SceneGraphNode &node, int parent = 0);

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
	bool trace(bool sceneMode, bool force = false, const glm::mat4 &invModel = glm::mat4(1.0f));
	void resetLastTrace();

	void startLocalServer(int port, const core::String &iface);
	void stopLocalServer();

	void connectToServer(const core::String &hostname, int port);
	void disconnectFromServer();

	bool setGridResolution(int resolution);

	scenegraph::SceneGraphNode *sceneGraphNode(int nodeId);
	const scenegraph::SceneGraphNode *sceneGraphNode(int nodeId) const;
	scenegraph::SceneGraphNode *sceneGraphModelNode(int nodeId);
	scenegraph::SceneGraphNode *sceneGraphNodeByUUID(const core::UUID &uuid);

	const voxel::ClipboardData &clipboardData() const;

	// component access
	const ModifierFacade &modifier() const;
	ModifierFacade &modifier();
	const memento::MementoHandler &mementoHandler() const;
	memento::MementoHandler &mementoHandler();
	const scenegraph::SceneGraph &sceneGraph() const;
	scenegraph::SceneGraph &sceneGraph();
	voxelgenerator::LUAApi &luaApi();
	Server &server();
	Client &client();

private:
	bool isValidReferenceNode(const scenegraph::SceneGraphNode &node) const;
	/**
	 * @brief When updating the pivot of a node, we want to keep it in its current position - but only modify the pivot
	 * to achieve this, we componsate the pivot change by updating the local translation
	 */
	void nodeSetPivot(scenegraph::SceneGraphNode &node, const glm::vec3 &pivot);

	void onNewNodeAdded(int newNodeId, bool isChildren = false);
	bool nodeRemoveChildrenByType(scenegraph::SceneGraphNode &node, scenegraph::SceneGraphNodeType type);
	bool nodeRename(scenegraph::SceneGraphNode &node, const core::String &name);
	bool nodeRemove(scenegraph::SceneGraphNode &node, bool recursive);
	bool nodeUpdateTransform(scenegraph::SceneGraphNode &node, const glm::mat4 &matrix,
							 scenegraph::KeyFrameIndex keyFrameIdx, bool local);
	bool nodeUpdateTransform(scenegraph::SceneGraphNode &node, const glm::vec3 &angles, const glm::vec3 &scale,
							 const glm::vec3 &translation, scenegraph::KeyFrameIndex keyFrameIdx, bool local);
	bool nodeResetTransform(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx);
	bool nodeTransformMirror(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx, math::Axis axis);
	bool nodeUpdateKeyFrameInterpolation(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx,
										 scenegraph::InterpolationType interpolation);
	bool nodeUpdatePivot(scenegraph::SceneGraphNode &node, const glm::vec3 &pivot);
	bool nodeRemoveKeyFrameByIndex(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx);
	bool nodeRemoveKeyFrame(scenegraph::SceneGraphNode &node, scenegraph::FrameIndex frameIdx);
	bool nodeAddKeyframe(scenegraph::SceneGraphNode &node, scenegraph::FrameIndex frameIdx);
	void nodeDuplicate(const scenegraph::SceneGraphNode &node, int *newNodeId = nullptr);
	int nodeReference(const scenegraph::SceneGraphNode &node);
	bool nodeUnreference(scenegraph::SceneGraphNode &node);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeDuplicateColor(scenegraph::SceneGraphNode &node, uint8_t palIdx);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeRemoveColor(scenegraph::SceneGraphNode &node, uint8_t palIdx);
	bool nodeReduceColors(scenegraph::SceneGraphNode &node, const core::Buffer<uint8_t> &srcPalIdx, uint8_t targetPalIdx);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeRemoveAlpha(scenegraph::SceneGraphNode &node, uint8_t palIdx);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeSetMaterial(scenegraph::SceneGraphNode &node, uint8_t palIdx, palette::MaterialProperty material,
						 float value);
	bool nodeSetColor(scenegraph::SceneGraphNode &node, uint8_t palIdx, const color::RGBA &color);
	bool nodeShiftAllKeyframes(scenegraph::SceneGraphNode &node, const glm::vec3 &shift);
	void nodeKeyFramesChanged(scenegraph::SceneGraphNode &node);
public:
	void nodeUpdatePartialVolume(scenegraph::SceneGraphNode &node, const voxel::RawVolume &volume);
	bool nodeUpdateTransform(int nodeId, const glm::vec3 &angles, const glm::vec3 &scale, const glm::vec3 &translation,
							 scenegraph::KeyFrameIndex keyFrameIdx, bool local);
	bool nodeUpdateTransformGroup(const glm::vec3 &angles, const glm::vec3 &scale, const glm::vec3 &translation,
								  scenegraph::FrameIndex frameIdx, bool local);
	bool nodeUpdateTransform(int nodeId, const glm::mat4 &matrix, scenegraph::KeyFrameIndex keyFrameIdx, bool local);
	bool nodeResetTransform(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx);
	bool nodeTransformMirror(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx, math::Axis axis);
	bool nodeUpdateKeyFrameInterpolation(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx,
										 scenegraph::InterpolationType interpolation);
	bool nodeUpdatePivotGroup(const glm::vec3 &pivot);
	bool nodeUpdatePivot(int nodeId, const glm::vec3 &pivot);
	bool nodeShiftAllKeyframes(int nodeId, const glm::vec3 &shift);
	bool nodeRemoveKeyFrameByIndex(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx);
	int nodeReference(int nodeId);
	bool nodeDuplicate(int nodeId, int *newNodeId = nullptr);
	bool nodeRemoveKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx);
	bool nodeAddKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx);
	bool nodeAllAddKeyFrames(scenegraph::FrameIndex frameIdx);
	bool nodeMove(int sourceNodeId, int targetNodeId, scenegraph::NodeMoveFlag flags);
	bool nodeSetProperty(int nodeId, const core::String &key, const core::String &value);
	bool nodeRemoveProperty(int nodeId, const core::String &key);
	bool nodeRename(int nodeId, const core::String &name);
	bool nodeRemove(int nodeId, bool recursive);
	bool nodeSetVisible(int nodeId, bool visible);
	bool nodeSetLocked(int nodeId, bool locked);
	bool nodeActivate(int nodeId);
	bool nodeUnreference(int nodeId);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeDuplicateColor(int nodeId, uint8_t palIdx);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeRemoveColor(int nodeId, uint8_t palIdx);
	/**
	 * @param[in] srcPalIdx The palette color indices to replace with the target palette index
	 * @param[in] targetPalIdx The target palette index
	 */
	bool nodeReduceColors(int nodeId, const core::Buffer<uint8_t> &srcPalIdx, uint8_t targetPalIdx);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeRemoveAlpha(int nodeId, uint8_t palIdx);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeSetMaterial(int nodeId, uint8_t palIdx, palette::MaterialProperty material, float value);
	bool nodeSetColor(int nodeId, uint8_t palIdx, const color::RGBA &color);
	void nodeResize(int nodeId, const voxel::Region &region);
	void nodeResize(int nodeId, const glm::ivec3 &size);
	/**
	 * @brief If a type of a palette color changes its transparency state, we have to update the voxels
	 * in the volume that are using this color. This is because we separate the color and the alpha voxels
	 * during mesh generation.
	 */
	void nodeUpdateVoxelType(int nodeId, uint8_t palIdx, voxel::VoxelType newType);
	/**
	 * @brief Shift the whole volume by the given world coordinates
	 */
	void nodeShift(int nodeId, const glm::ivec3 &m);
	/**
	 * @brief Move the voxels inside the volume regions
	 */
	void nodeMoveVoxels(int nodeId, const glm::ivec3 &m);
	/**
	 * @brief Remove unused colors from the palette of the given node
	 *
	 * @param[in] reindexPalette If @c true the palette will be reindexed after removing the unused colors to remove
	 * gaps. This will also update the voxels.
	 */
	void nodeRemoveUnusedColors(int nodeId, bool reindexPalette = false);
	/**
	 * @note This is not related to the group node type
	 */
	void nodeForeachGroup(const std::function<void(int)> &f);

	const voxelrender::CameraMovement &cameraMovement() const;
	voxelrender::CameraMovement &cameraMovement();
};

inline const voxelrender::CameraMovement &SceneManager::cameraMovement() const {
	return _camMovement;
}

inline voxelrender::CameraMovement &SceneManager::cameraMovement() {
	return _camMovement;
}

inline Server &SceneManager::server() {
	return _server;
}

inline Client &SceneManager::client() {
	return _client;
}

inline const voxel::ClipboardData &SceneManager::clipboardData() const {
	return _copy;
}

inline scenegraph::FrameIndex SceneManager::currentFrame() const {
	return _currentFrameIdx;
}

inline void SceneManager::setCurrentFrame(scenegraph::FrameIndex frameIdx) {
	_currentFrameIdx = frameIdx;
}

inline video::Camera *SceneManager::activeCamera() const {
	return _camera;
}

inline const memento::MementoHandler &SceneManager::mementoHandler() const {
	return _mementoHandler;
}

inline memento::MementoHandler &SceneManager::mementoHandler() {
	return _mementoHandler;
}

inline bool SceneManager::dirty() const {
	return _dirty;
}

inline int SceneManager::size() const {
	return _size;
}

inline const voxel::Voxel &SceneManager::hitCursorVoxel() const {
	return _modifierFacade.hitCursorVoxel();
}

inline const glm::ivec3 &SceneManager::cursorPosition() const {
	return _modifierFacade.cursorPosition();
}

inline const glm::ivec3 &SceneManager::referencePosition() const {
	return _modifierFacade.referencePosition();
}

inline const ModifierFacade &SceneManager::modifier() const {
	return _modifierFacade;
}

inline ModifierFacade &SceneManager::modifier() {
	return _modifierFacade;
}

inline voxelgenerator::LUAApi &SceneManager::luaApi() {
	return _luaApi;
}

using SceneManagerPtr = core::SharedPtr<SceneManager>;

} // namespace voxedit
