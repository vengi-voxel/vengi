/**
 * @file
 */

#pragma once

#include "ISceneRenderer.h"
#include "LUAApiListener.h"
#include "SceneJob.h"
#include "command/ActionButton.h"
#include "core/DeltaFrameSeconds.h"
#include "core/SharedProgress.h"
#include "core/SharedPtr.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "core/concurrent/Future.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "modifier/SceneModifiedFlags.h"
#include "scenegraph/SceneGraphNodeValueCache.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/network/Client.h"
#include "voxedit-util/network/Server.h"
#include "voxedit-util/network/SessionRecorder.h"
#include "voxedit-util/network/SessionPlayer.h"
#include "voxel/ClipboardData.h"
#include "voxel/Connectivity.h"
#include "voxel/Region.h"
#include "image/ImageFwd.h"
#include "memento/MementoHandler.h"

namespace command {
class CommandArgs;
}

namespace voxel {
enum class FaceNames : uint8_t;
}

namespace voxelgenerator {
class LUAApi;
namespace lsystem {
struct LSystemConfig;
}
}

namespace video {
class Camera;
}

namespace voxelrender {
class CameraMovement;
}

namespace sound {
class SoundManager;
using SoundHandle = void *;
}

namespace voxelutil {
struct PickResult;
}

namespace voxedit {

class IModifierRenderer;
class Modifier;
struct LSystemRuntime;
struct AddNodePreview;
using ModifierRendererPtr = core::SharedPtr<IModifierRenderer>;

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

class SceneManager;

/**
 * @brief Hold shift over the scene gizmo for create-reference mode; shift+click creates the node.
 */
class CreateReferenceButton : public command::ActionButton {
private:
	using Super = command::ActionButton;
	SceneManager *_sceneMgr;

public:
	CreateReferenceButton(SceneManager *sceneMgr);
	bool handleDown(int32_t key, double pressedSeconds) override;
};

/**
 * @brief Owns and coordinates the editable voxel scene.
 *
 * SceneManager is the central mutation boundary for voxedit: it owns the scene
 * graph, memento state, modifier state, networking hooks and background scene
 * jobs. Long running volume operations are snapshotted here, computed on a
 * worker thread and applied back on the main thread to keep live scene graph
 * state single-threaded.
 *
 * @note The data is shared across all viewports.
 */
class SceneManager : public core::DeltaFrameSeconds {
	friend class LUAApiListener;
	friend class CreateReferenceButton;

protected:
	scenegraph::SceneGraph _sceneGraph;
	voxelrender::CameraMovement *_camMovement = nullptr;
	memento::MementoHandler *_mementoHandler = nullptr;
	voxel::ClipboardData _copy;
	core::Future<scenegraph::SceneGraph> _loadingFuture;
	core::SharedProgress _loadingProgress;
	core::Future<SceneJobResult> _sceneJobFuture;
	core::DynamicArray<SceneJobRequest> _sceneJobQueue;
	SceneJobType _sceneJobType = SceneJobType::None;
	core::String _sceneJobText;
	core::SharedProgress _sceneJobProgress;
	bool _sceneJobCancelRequested = false;
	core::TimeProviderPtr _timeProvider;
	SceneRendererPtr _sceneRenderer;
	Modifier *_modifier = nullptr;
	voxelgenerator::LUAApi *_luaApi = nullptr;
	LUAApiListener _luaApiListener;
	io::FilesystemPtr _filesystem;
	Server _server;
	Client _client;
	SessionRecorder _recorder;
	SessionPlayer _player;
	sound::SoundManager *_soundManager = nullptr;
	sound::SoundHandle _chatSound = nullptr;

	/**
	 * The @c video::Camera instance of the currently active @c Viewport
	 */
	video::Camera *_camera = nullptr;
	bool _fixedCamera = false;

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

	LSystemRuntime *_lsystem = nullptr;
	bool _lsystemRunning = false;

	io::FileDescription _lastFilename;
	double _lastAutoSave = 0u;

	int _lastRaytraceX = -1;
	int _lastRaytraceY = -1;

	// model animation speed
	double _frameAnimationSpeed = 0.0;
	double _nextFrameSwitch = 0.0;
	int _frameAnimationNodeId = InvalidNodeId;
	bool _animationResetCamera = false;

	// timeline animation
	scenegraph::FrameIndex _currentFrameIdx = 0;

	int _initialized = 0;
	glm::ivec2 _mouseCursor{0};
	glm::ivec2 _mouseCursorDelta{0};
	bool _mouseLookActive = false;
	uint8_t _preMouselookRotationType = 0; // video::CameraRotationType::Target

	command::ActionButton _move[lengthof(DIRECTIONS)];
	command::ActionButton _rotate;
	command::ActionButton _pan;
	command::ActionButton _zoomIn;
	command::ActionButton _zoomOut;
	command::ActionButton _toggleNodeAdd; // add-node-by-face mode (shift in scene)
	CreateReferenceButton _createReference;

	voxelutil::PickResult *_result = nullptr;

	AddNodePreview *_addNodePreview = nullptr;
	core::VarPtr _addNodeMode;
	core::VarPtr _addNodeIgnoreOverlap;
	core::VarPtr _addNodeCloneVoxels;
	bool _viewportGizmoActive = false;
	bool _viewportHudHovered = false;

	mutable SceneGraphNodeValueCache<voxel::Region> _selectionRegionCache;

	void autoSelectSolidVoxels(scenegraph::SceneGraphNode *node, const voxel::Region &region);
	bool loadGlobalClipboard(voxel::ClipboardData &clipData);
	/**
	 * @brief Create a reference of the active model node (used by CreateReferenceButton)
	 */
	void createReferenceFromGizmo();

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
	bool setNewVolume(const core::UUID &nodeUUID, voxel::RawVolume *volume, bool deleteMesh = true);
	void modified(int nodeId, const voxel::Region &modifiedRegion, SceneModifiedFlags flags = SceneModifiedFlags::All,
				  uint64_t renderRegionMillis = 0);
	voxel::RawVolume *volume(int nodeId);
	const voxel::RawVolume *volume(int nodeId) const;
	int addModelAdjacent(int sourceNodeId, voxel::FaceNames face);
	int mergeNodes(int nodeId1, int nodeId2);
	bool nodeCopy(int nodeId);
	bool nodePaste(int nodeId, const glm::ivec3 &pos);
	bool nodeGlobalCopy(int nodeId);
	bool nodeGlobalPaste(int nodeId, const glm::ivec3 &pos);
	bool splatMerge(int sourceNodeId);
	bool nodeCalculateNormals(int nodeId, voxel::Connectivity connectivity, bool recalcAll = false,
							  bool fillAndHollow = false);
	bool nodePasteAsNewNode(int nodeId);
	bool nodeCut(int nodeId);
	bool nodeUpdateTransform(int nodeId, const glm::vec3 &angles, const glm::vec3 &scale,
							 const glm::vec3 &translation, scenegraph::KeyFrameIndex keyFrameIdx, bool local);
	bool nodeUpdateTransform(int nodeId, const glm::mat4 &matrix, scenegraph::KeyFrameIndex keyFrameIdx, bool local);
	bool nodeResetTransform(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx);
	bool nodeTransformMirror(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx, math::Axis axis);
	bool nodeUpdateKeyFrameInterpolation(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx,
										 scenegraph::InterpolationType interpolation);
	bool nodeUpdatePivot(int nodeId, const glm::vec3 &pivot);
	bool nodeShiftAllKeyframes(int nodeId, const glm::vec3 &shift);
	bool nodeRemoveKeyFrameByIndex(int nodeId, scenegraph::KeyFrameIndex keyFrameIdx);
	int nodeReference(int nodeId);
	bool nodeDuplicate(int nodeId, int *newNodeId = nullptr);
	bool nodeRemoveKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx);
	bool nodeAddKeyFrame(int nodeId, scenegraph::FrameIndex frameIdx);
	bool nodeMove(int sourceNodeId, int targetNodeId, scenegraph::NodeMoveFlag flags);
	bool nodeSetProperty(int nodeId, const core::String &key, const core::String &value);
	bool nodeRemoveProperty(int nodeId, const core::String &key);
	bool nodeSetIKConstraint(int nodeId, const scenegraph::IKConstraint &constraint);
	bool nodeRemoveIKConstraint(int nodeId);
	bool nodeRename(int nodeId, const core::String &name);
	bool nodeRemove(int nodeId, bool recursive);
	bool nodeSetVisible(int nodeId, bool visible);
	bool nodeSetLocked(int nodeId, bool locked);
	bool nodeSetOpacity(int nodeId, float opacity);
	bool nodeActivate(int nodeId);
	bool nodeUnreference(int nodeId);
	bool nodeRemoveNormals(int nodeId);
	bool nodeDuplicateColor(int nodeId, uint8_t palIdx);
	bool nodeRemoveColor(int nodeId, uint8_t palIdx);
	bool nodeReduceColors(int nodeId, const core::Buffer<uint8_t> &srcPalIdx, uint8_t targetPalIdx);
	bool nodeQuantizeColors(int nodeId, const core::Buffer<uint8_t> &selectedIndices, int targetColorCount);
	bool nodeRemoveAlpha(int nodeId, uint8_t palIdx);
	bool nodeResetMaterial(int nodeId, uint8_t palIdx);
	bool nodeSetMaterial(int nodeId, uint8_t palIdx, palette::MaterialProperty material, float value);
	bool nodeSetColor(int nodeId, uint8_t palIdx, const color::RGBA &color);
	void nodeResize(int nodeId, const voxel::Region &region);
	void nodeResize(int nodeId, const glm::ivec3 &size);
	void nodeRescaleContent(int nodeId, const glm::ivec3 &targetSize);
	void nodeBakeTransform(int nodeId);
	void nodeUpdateVoxelType(int nodeId, uint8_t palIdx, voxel::VoxelType newType);
	void nodeShift(int nodeId, const glm::ivec3 &m);
	void nodeMoveVoxels(int nodeId, const glm::ivec3 &m);
	void nodeRemoveUnusedColors(int nodeId, bool reindexPalette = false);
	void autosave();
	void setReferencePosition(const glm::ivec3 &pos);
	void updateDirtyRendererStates();
	bool mouseRayTrace(bool force, const glm::mat4 &invModel);
	void updateCursor();
	int traceScene(bool skipActiveNode = true);
	void stepLSystem();
	int toNodeId(const command::CommandArgs& args, int defaultVal, const core::String &name = "nodeid") const;
	core::UUID toNodeUUID(const command::CommandArgs &args, const core::UUID &defaultVal,
						  const core::String &name = "nodeid") const;

	bool setSceneGraphNodeVolume(scenegraph::SceneGraphNode &node, voxel::RawVolume *volume);
	bool startSceneJob(SceneJobRequest &&request);
	bool startSceneJob(SceneJobType type, int nodeId);
	bool startSceneJob(SceneJobType type, const core::UUID &nodeUUID);
	bool startActiveSceneJob(SceneJobType type, const core::String &text, core::Future<SceneJobResult> &&future);
	bool startVolumeOperationSceneJob(const SceneJobRequest &request);
	bool startCropSceneJob(const core::UUID &nodeUUID, const core::String &text);
	bool startScaleUpSceneJob(const core::UUID &nodeUUID, const core::String &text);
	bool startScaleDownSceneJob(const core::UUID &nodeUUID, const core::String &text);
	bool startResizeSceneJob(const SceneJobRequest &request);
	bool startSplitObjectsSceneJob(const core::UUID &nodeUUID, const core::String &text);
	bool startColorToModelSceneJob(const SceneJobRequest &request);
	bool startSplatMergeSceneJob(const core::UUID &nodeUUID, const core::String &text);
	void startNextQueuedSceneJob();
	void updateSceneJob();
	bool applySceneJobResult(SceneJobResult &&result);
	bool queueSceneJobForGroup(SceneJobType type);

	void animateFrames(double nowSeconds);
	/**
	 * @brief Move the cursor relative by the given steps in each direction
	 */
	void moveCursor(int x, int y, int z);

	void nodeGroupFillHollow();
	void nodeGroupHollow();
	void nodeGroupFill();
	void nodeGroupClear();
	void nodeGroupDeleteSelected();
	void nodeGroupColorSelected(uint8_t colorIndex);
	void nodeGroupFilterSelection(uint8_t colorIndex, bool deselectMatching);
	void nodeGroupSelectByAirAxes(int minAxes);
	void nodeGroupDeselectColor(uint8_t colorIndex);
	void nodeGroupSelectOnlyColor(uint8_t colorIndex);
	void nodeGroupSelectOnlyEdges();
	void nodeGroupSelectOnlyCorners();
	void nodeGroupSelectOnlyWallEdges();
	void nodeGroupSelectionGrow();
	void nodeGroupRotate(math::Axis axis);
	void nodeGroupFlip(math::Axis axis);
	void nodeGroupResize(const glm::ivec3 &size);

	int nodeColorToNewNode(int nodeId, const voxel::Voxel voxelColor);
	int nodeColorToNewNode(const core::UUID &nodeUUID, const voxel::Voxel voxelColor);
	int nodeColorToNewNode(const voxel::Voxel voxelColor);
	int nodeColorToNewNode(int nodeId, const core::Buffer<uint8_t> &paletteIndices);
	int nodeColorToNewNode(const core::UUID &nodeUUID, const core::Buffer<uint8_t> &paletteIndices);
	void nodeCrop(int nodeId);
	void nodeCrop(const core::UUID &nodeUUID);
	void nodeSplitObjects(int nodeId);
	void nodeSplitObjects(const core::UUID &nodeUUID);
	void nodeScaleDown(int nodeId);
	void nodeScaleDown(const core::UUID &nodeUUID);
	void nodeScaleUp(int nodeId);
	void nodeScaleUp(const core::UUID &nodeUUID);
	bool nodeSave(int nodeId, const core::String &file);
	bool nodeSave(const core::UUID &nodeUUID, const core::String &file);
	void nodeRotateAll(math::Axis axis);

	bool doUndo();
	bool doRedo();

	bool saveModels(const core::String &dir);

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
	bool mementoIKConstraint(const memento::MementoState &s);
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
	bool nodeQuantizeColors(scenegraph::SceneGraphNode &node, const core::Buffer<uint8_t> &selectedIndices, int targetColorCount);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeRemoveAlpha(scenegraph::SceneGraphNode &node, uint8_t palIdx);
	bool nodeResetMaterial(scenegraph::SceneGraphNode &node, uint8_t palIdx);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeSetMaterial(scenegraph::SceneGraphNode &node, uint8_t palIdx, palette::MaterialProperty material,
						 float value);
	bool nodeSetColor(scenegraph::SceneGraphNode &node, uint8_t palIdx, const color::RGBA &color);
	bool nodeShiftAllKeyframes(scenegraph::SceneGraphNode &node, const glm::vec3 &shift);
	void nodeKeyFramesChanged(scenegraph::SceneGraphNode &node);
	voxel::Region selectionCalculateRegion(const scenegraph::SceneGraphNode &node) const;
	voxel::ClipboardData nodeClipboardCopy(scenegraph::SceneGraphNode &node);

public:
	SceneManager(const core::TimeProviderPtr &timeProvider, const io::FilesystemPtr &filesystem,
				 const SceneRendererPtr &sceneRenderer, const ModifierRendererPtr &modifierRenderer);
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
	const glm::ivec2 &mousePos() const;
	void setMouseLook(bool active);
	void setMouseDelta(int dx, int dy);

	/**
	 * @brief world matrix for the current active node
	 */
	glm::mat4 worldMatrix(scenegraph::FrameIndex frameIdx = 0, bool applyTransforms = true) const;

	const voxel::Region &sliceRegion() const;
	void setSliceRegion(const voxel::Region &region);
	bool isSliceModeActive() const;

	bool exceedsMaxSuggestedVolumeSize() const;
	bool exceedsMaxSuggestedVolumeSize(const voxel::Region &region) const;

	scenegraph::SceneGraphNodeCamera *activeCameraNode();

	scenegraph::FrameIndex currentFrame() const;
	void setCurrentFrame(scenegraph::FrameIndex frameIdx);

	bool setAnimation(const core::String &animation);
	bool addAnimation(const core::String &animation);
	bool duplicateAnimation(const core::String &animation, const core::String &newName);
	bool removeAnimation(const core::String &animation);

	void setActiveCamera(video::Camera *camera, bool fixedCamera);
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

	void modified(const core::UUID &nodeUUID, const voxel::Region &modifiedRegion,
				  SceneModifiedFlags flags = SceneModifiedFlags::All, uint64_t renderRegionMillis = 0);
	voxel::RawVolume *volume(const core::UUID &nodeUUID);
	const voxel::RawVolume *volume(const core::UUID &nodeUUID) const;
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

	/**
	 * @brief Add a new model node as children to the current active node
	 */
	int addModelChild(const core::String &name, int width, int height, int depth, const core::UUID &uuid = core::UUID());
	/**
	 * @brief Add a new model node adjacent to the given node on the given face
	 */
	int addModelAdjacent(const core::UUID &sourceNodeUUID, voxel::FaceNames face);
	int addPointChild(const core::String &name, const glm::ivec3 &position, const glm::quat &orientation,
					  const core::UUID &uuid = core::UUID());

	bool isAddNodeModeActive() const;
	void setAddNodeModeActive(bool active);
	void toggleAddNodeMode();
	/**
	 * @return @c true while the key bound to @c +createreference is held
	 */
	bool isCreateReferencePressed() const;
	void updateAddNodeHover(scenegraph::FrameIndex frameIdx);
	const AddNodePreview &addNodePreview() const;
	bool blocksSceneMouseInteraction() const;
	void resetViewportMouseBlock();
	bool isViewportGizmoActive() const;
	void setViewportGizmoActive(bool active);
	void setViewportHudHovered(bool hovered);

	/**
	 * @brief Merge two nodes and extend the smaller one
	 */
	int mergeNodes(const core::UUID &nodeUUID1, const core::UUID &nodeUUID2);
	int mergeNodes(NodeMergeFlags flags);

	/**
	 * @brief Split volumes into smaller volumes to improve performance
	 */
	bool splitVolumes();

	bool nodeCopy(const core::UUID &nodeUUID);
	bool nodePaste(const core::UUID &nodeUUID, const glm::ivec3& pos);
	bool nodeGlobalCopy(const core::UUID &nodeUUID);
	bool nodeGlobalPaste(const core::UUID &nodeUUID, const glm::ivec3 &pos);

	bool paste(const glm::ivec3 &pos);
	bool globalPaste(const glm::ivec3 &pos);
	bool globalCopy();
	bool globalCopyVisible();
	bool globalPasteNode(const glm::ivec3 &pos);

	/**
	 * @brief Splats (merges) a source node into all other intersecting nodes in the scene.
	 *
	 * Projects the source node's voxels into world space and overwrites voxels in any
	 * intersecting background or model nodes.
	 *
	 * @param sourceNodeId The ID of the node to merge into the scene.
	 * @return @c true if the merge was successful, @c false otherwise.
	 */
	bool splatMerge(const core::UUID &sourceNodeUUID);

	/**
	 * @brief Merges the currently active node into the background structure.
	 *
	 * Slices the active node into chunks matching the background layer structure
	 * (or modifies underlying background nodes if they already exist).
	 *
	 * @return @c true if the operation succeeded, @c false otherwise.
	 */
	bool mergeActiveToBackground();

	/**
	 * @brief Merges all visible model nodes into a single temporary combined node.
	 *
	 * This does not modify the existing visible nodes, but creates a new, temporary node
	 * containing the baked world-space volume of all visible nodes.
	 *
	 * @return The node ID of the newly created temporary node, or @c InvalidNodeId if no merge was performed.
	 */
	int mergeVisibleToTemp();

	void selectionInvert(const core::UUID &nodeUUID);
	void selectionUnselect(const core::UUID &nodeUUID);
	void selectionSelectAll(const core::UUID &nodeUUID);
	void selectionSetBounds(const core::UUID &nodeUUID, const voxel::Region &region);
	void selectionSetEllipse(const core::UUID &nodeUUID);
	bool hasSelection(const core::UUID &nodeUUID) const;
	bool isSelected(const core::UUID &nodeUUID, const glm::ivec3 &pos) const;
	voxel::Region selectionCalculateRegion(const core::UUID &nodeUUID) const;

	void lsystem(const voxelgenerator::lsystem::LSystemConfig &conf);
	void lsystemAbort();
	bool lsystemRunning() const;
	float lsystemProgress() const;

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
	/**
	 * @brief Normalized load progress in [0, 1] while @c isLoading() is true.
	 */
	float loadingProgress() const;
	/**
	 * @brief Optional status text from the active format loader (e.g. mesh name).
	 */
	core::String loadingProgressText() const;
	bool loadSceneGraph(scenegraph::SceneGraph &&sceneGraph, bool disconnect = true);

	/**
	 * @brief Returns @c true if the scene is locked and no modifications should be made.
	 */
	bool isLocked() const;

	/**
	 * @brief Returns @c true if a background scene job is currently running.
	 */
	bool isCommandRunning() const;
	bool isSceneJobRunning() const;
	const core::String &sceneJobText() const;
	/**
	 * @brief Normalized scene-job progress in [0, 1] while @c isSceneJobRunning() is true.
	 */
	float sceneJobProgress() const;
	/**
	 * @brief Optional status text from the active scene job (falls back to @c sceneJobText()).
	 */
	core::String sceneJobProgressText() const;
	bool cancelSceneJob();
	bool cancelPendingSceneJob(int index);
	void clearPendingSceneJobs();
	int pendingSceneJobs() const;
	const core::String &pendingSceneJobText(int index) const;

	bool nodeResizeAsync(const core::UUID &nodeUUID, const voxel::Region &region);
	bool nodeResizeAsync(const core::UUID &nodeUUID, const glm::ivec3 &size);
	bool nodeColorToNewNodeAsync(const core::UUID &nodeUUID, const core::Buffer<uint8_t> &paletteIndices);

	bool undo(int n = 1);
	bool redo(int n = 1);

	/**
	 * @brief Import an existing model
	 * @note Placed relative to the reference position in the current scene
	 */
	bool import(const core::String &file);
	bool importDirectory(const core::String &directory, const io::FormatDescription *format = nullptr, int depth = 3);

	bool isScriptRunning() const;
	bool runScript(const core::String &luaCode, const core::DynamicArray<core::String> &args);
	/**
	 * @brief Run a lua script synchronously to completion, including all coroutine yields.
	 * This is used by the MCP server to execute scripts and wait for them to finish.
	 */
	bool runScriptSync(const core::String &luaCode, const core::DynamicArray<core::String> &args);

	/**
	 * @brief Take ownership of the volume if this returns @c true, otherwise the caller must free the memory
	 */
	bool newScene(bool force, const core::String &name, voxel::RawVolume *v);
	bool newScene(bool force, const core::String &name, const voxel::Region &region);
	int moveNodeToSceneGraph(scenegraph::SceneGraphNode &node, int parent = 0);

	// Called by Regrid after bulk add/remove operations that bypass moveNodeToSceneGraph
	// and nodeRemove to avoid O(N^2) updateTransforms. Notifies the renderer and marks dirty.
	void onRegridComplete(const core::DynamicArray<int> &newCellIds, const core::DynamicArray<int> &deletedSourceIds);

	/**
	 * @return @c true if the scene was modified and not saved yet
	 */
	bool dirty() const;
	void markDirty();
	void clearDirty();

	/**
	 * @return @c true if the scene is completely empty
	 */
	bool empty() const;

	/**
	 * @note This is not about the animation scene mode, but the animation of the nodes
	 */
	bool frameAnimationActive() const;

	static const uint8_t RenderScene = 1u << 0u;
	static const uint8_t RenderUI = 1u << 1u;
	static const uint8_t RenderAll = RenderScene | RenderUI;

	/**
	 * @brief Performs the rendering for each @c Viewport instance
	 */
	void render(voxelrender::RenderContext &renderContext, voxelrender::RenderContext &modifierRenderContext, const video::Camera &camera, uint8_t renderMask = RenderAll);

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

	bool connectToServer(const core::String &hostname, int port);
	void disconnectFromServer();

	bool startRecording(const core::String &filename);
	void stopRecording();
	bool isRecording() const;

	bool startPlayback(const core::String &filename);
	void stopPlayback();
	bool isPlaying() const;
	bool isPlaybackPaused() const;
	void setPlaybackPaused(bool paused);
	float playbackSpeed() const;
	void setPlaybackSpeed(float speed);

	bool setGridResolution(const glm::ivec3 &resolution);
	bool setGridResolution(int resolution);

	int activeNode() const;
	const core::UUID &activeNodeUUID() const;

	scenegraph::SceneGraphNode *sceneGraphNode(int nodeId);
	const scenegraph::SceneGraphNode *sceneGraphNode(int nodeId) const;
	scenegraph::SceneGraphNode *sceneGraphModelNode(int nodeId);
	const scenegraph::SceneGraphNode *sceneGraphModelNode(int nodeId) const;
	scenegraph::SceneGraphNode *sceneGraphNodeByUUID(const core::UUID &uuid);
	const scenegraph::SceneGraphNode *sceneGraphNodeByUUID(const core::UUID &uuid) const;
	scenegraph::SceneGraphNode *sceneGraphModelNodeByUUID(const core::UUID &uuid);
	const scenegraph::SceneGraphNode *sceneGraphModelNodeByUUID(const core::UUID &uuid) const;

	const voxel::ClipboardData &clipboardData() const;

	// component access
	const Modifier &modifier() const;
	Modifier &modifier();
	const memento::MementoHandler &mementoHandler() const;
	memento::MementoHandler &mementoHandler();
	const scenegraph::SceneGraph &sceneGraph() const;
	scenegraph::SceneGraph &sceneGraph();
	voxelgenerator::LUAApi &luaApi();
	Server &server();
	Client &client();
	SessionRecorder &recorder();
	SessionPlayer &player();
	sound::SoundManager &soundManager();
	sound::SoundHandle chatSound() const;
	const voxelrender::CameraMovement &cameraMovement() const;
	voxelrender::CameraMovement &cameraMovement();

	void nodeGroupResetTransform(scenegraph::KeyFrameIndex keyFrameIdx);
	bool nodeGroupUpdatePivot(const glm::vec3 &pivot);
	void nodeGroupRemoveKeyFrame(scenegraph::FrameIndex frameIdx);
	void nodeGroupAddKeyFrame(scenegraph::FrameIndex frameIdx);
	bool nodeGroupUpdateTransform(const glm::vec3 &angles, const glm::vec3 &scale, const glm::vec3 &translation,
								  scenegraph::FrameIndex frameIdx, bool local);
	/**
	 * @brief Shift the whole volume by the given voxel amount
	 */
	void nodeGroupShift(int x, int y, int z);
	/**
	 * @brief Move the voxels inside the volume regions
	 */
	void nodeGroupMoveVoxels(int x, int y, int z);

	void nodeGroupCalulateNormals(voxel::Connectivity connectivity, bool recalcAll, bool fillAndHollow);
	bool nodeCalculateNormals(const core::UUID &nodeUUID, voxel::Connectivity connectivity, bool recalcAll = false,
							  bool fillAndHollow = false);

	bool nodePasteAsNewNode(const core::UUID &nodeUUID);
	bool nodeCut(const core::UUID &nodeUUID);

	void nodeUpdatePartialVolume(scenegraph::SceneGraphNode &node, const voxel::RawVolume &volume);
	bool nodeUpdateTransform(const core::UUID &nodeUUID, const glm::vec3 &angles, const glm::vec3 &scale,
							 const glm::vec3 &translation, scenegraph::KeyFrameIndex keyFrameIdx, bool local);
	bool nodeUpdateTransform(const core::UUID &nodeUUID, const glm::mat4 &matrix, scenegraph::KeyFrameIndex keyFrameIdx,
							 bool local);
	bool nodeResetTransform(const core::UUID &nodeUUID, scenegraph::KeyFrameIndex keyFrameIdx);
	bool nodeTransformMirror(const core::UUID &nodeUUID, scenegraph::KeyFrameIndex keyFrameIdx, math::Axis axis);
	bool nodeUpdateKeyFrameInterpolation(const core::UUID &nodeUUID, scenegraph::KeyFrameIndex keyFrameIdx,
										 scenegraph::InterpolationType interpolation);
	bool nodeUpdatePivot(const core::UUID &nodeUUID, const glm::vec3 &pivot);
	bool nodeShiftAllKeyframes(const core::UUID &nodeUUID, const glm::vec3 &shift);
	bool nodeRemoveKeyFrameByIndex(const core::UUID &nodeUUID, scenegraph::KeyFrameIndex keyFrameIdx);
	int nodeReference(const core::UUID &nodeUUID);
	bool nodeDuplicate(const core::UUID &nodeUUID, core::UUID *newNodeUUID = nullptr);
	bool nodeRemoveKeyFrame(const core::UUID &nodeUUID, scenegraph::FrameIndex frameIdx);
	bool nodeAddKeyFrame(const core::UUID &nodeUUID, scenegraph::FrameIndex frameIdx);
	bool nodeAllAddKeyFrames(scenegraph::FrameIndex frameIdx);
	bool nodeMove(const core::UUID &sourceNodeUUID, const core::UUID &targetNodeUUID, scenegraph::NodeMoveFlag flags);
	bool nodeSetProperty(const core::UUID &nodeUUID, const core::String &key, const core::String &value);
	bool nodeRemoveProperty(const core::UUID &nodeUUID, const core::String &key);
	bool nodeSetIKConstraint(const core::UUID &nodeUUID, const scenegraph::IKConstraint &constraint);
	bool nodeRemoveIKConstraint(const core::UUID &nodeUUID);
	bool nodeRename(const core::UUID &nodeUUID, const core::String &name);
	bool nodeRemove(const core::UUID &nodeUUID, bool recursive);
	bool nodeSetVisible(const core::UUID &nodeUUID, bool visible);
	bool nodeSetLocked(const core::UUID &nodeUUID, bool locked);
	bool nodeSetOpacity(const core::UUID &nodeUUID, float opacity);
	bool nodeActivate(const core::UUID &nodeUUID);
	bool nodeUnreference(const core::UUID &nodeUUID);
	void nodeGroupRemoveNormals();
	bool nodeRemoveNormals(const core::UUID &nodeUUID);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeDuplicateColor(const core::UUID &nodeUUID, uint8_t palIdx);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeRemoveColor(const core::UUID &nodeUUID, uint8_t palIdx);
	/**
	 * @param[in] srcPalIdx The palette color indices to replace with the target palette index
	 * @param[in] targetPalIdx The target palette index
	 */
	bool nodeReduceColors(const core::UUID &nodeUUID, const core::Buffer<uint8_t> &srcPalIdx, uint8_t targetPalIdx);
	bool nodeQuantizeColors(const core::UUID &nodeUUID, const core::Buffer<uint8_t> &selectedIndices, int targetColorCount);
	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeRemoveAlpha(const core::UUID &nodeUUID, uint8_t palIdx);
	bool nodeResetMaterial(const core::UUID &nodeUUID, uint8_t palIdx);

	/**
	 * @param[in] palIdx The visual palette index (this is **not** the real color index, but the index of the visual
	 * representation of the palette)
	 */
	bool nodeSetMaterial(const core::UUID &nodeUUID, uint8_t palIdx, palette::MaterialProperty material, float value);
	bool nodeSetColor(const core::UUID &nodeUUID, uint8_t palIdx, const color::RGBA &color);
	void nodeResize(const core::UUID &nodeUUID, const voxel::Region &region);
	void nodeResize(const core::UUID &nodeUUID, const glm::ivec3 &size);
	void nodeRescaleContent(const core::UUID &nodeUUID, const glm::ivec3 &targetSize);
	void nodeBakeTransform(const core::UUID &nodeUUID);
	/**
	 * @brief If a type of a palette color changes its transparency state, we have to update the voxels
	 * in the volume that are using this color. This is because we separate the color and the alpha voxels
	 * during mesh generation.
	 */
	void nodeUpdateVoxelType(const core::UUID &nodeUUID, uint8_t palIdx, voxel::VoxelType newType);
	/**
	 * @brief Shift the whole volume by the given world coordinates
	 */
	void nodeShift(const core::UUID &nodeUUID, const glm::ivec3 &m);
	/**
	 * @brief Move the voxels inside the volume regions
	 */
	void nodeMoveVoxels(const core::UUID &nodeUUID, const glm::ivec3 &m);
	/**
	 * @brief Remove unused colors from the palette of the given node
	 *
	 * @param[in] reindexPalette If @c true the palette will be reindexed after removing the unused colors to remove
	 * gaps. This will also update the voxels.
	 */
	void nodeRemoveUnusedColors(const core::UUID &nodeUUID, bool reindexPalette = false);
	/**
	 * @note This is not related to the group node type
	 * @note Visitor may accept @c int, @c core::UUID, or @c scenegraph::SceneGraphNode&
	 *       (see scenegraph::SceneGraph::foreachGroup).
	 */
	template<class FUNC>
	void nodeForeachGroup(FUNC &&f) {
		memento::ScopedMementoGroup mementoGroup(*_mementoHandler, "group");
		_sceneGraph.foreachGroup(core::forward<FUNC>(f));
	}
};

inline Server &SceneManager::server() {
	return _server;
}

inline Client &SceneManager::client() {
	return _client;
}

inline SessionRecorder &SceneManager::recorder() {
	return _recorder;
}

inline SessionPlayer &SceneManager::player() {
	return _player;
}

inline sound::SoundHandle SceneManager::chatSound() const {
	return _chatSound;
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

inline bool SceneManager::dirty() const {
	return _dirty;
}

inline void SceneManager::clearDirty() {
	_dirty = false;
}

inline bool SceneManager::lsystemRunning() const {
	return _lsystemRunning;
}

inline const glm::ivec2 &SceneManager::mousePos() const {
	return _mouseCursor;
}

using SceneManagerPtr = core::SharedPtr<SceneManager>;

} // namespace voxedit
