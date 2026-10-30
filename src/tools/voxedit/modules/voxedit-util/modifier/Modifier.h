/**
 * @file
 */

#pragma once

#include "BrushApply.h"
#include "BrushDraft.h"
#include "BrushEditSession.h"
#include "BrushRegistry.h"
#include "IModifierRenderer.h"
#include "ModifierButton.h"
#include "ModifierType.h"
#include "PreviewManager.h"
#include "SceneModifiedFlags.h"
#include "ScriptManager.h"
#include "brush/Brush.h"
#include "brush/BrushType.h"
#include "brush/LineBrush.h"
#include "brush/LUASelectionMode.h"
#include "brush/PaintBrush.h"
#include "brush/PlaneBrush.h"
#include "brush/ExtrudeBrush.h"
#include "brush/SelectBrush.h"
#include "brush/TransformBrush.h"
#include "brush/RulerBrush.h"
#include "brush/SculptBrush.h"
#include "brush/ShapeBrush.h"
#include "brush/StampBrush.h"
#include "brush/TextBrush.h"
#include "brush/TextureBrush.h"
#include "core/IComponent.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/brush/NormalBrush.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"

namespace palette {
class Palette;
}

namespace video {
class Camera;
}

namespace voxelrender {
struct RenderContext;
}

namespace voxedit {

class SceneManager;

/**
 * @brief Owns brushes, the live input context, and edit lifecycle.
 *
 * Registered tools use the shared sample evaluator or an isolated typed draft.
 * Input adapters forward events; the host owns targets, completion and history.
 */
class Modifier : public core::IComponent {
public:
	using ModifiedRegionCallback =
		core::Function<void(const voxel::Region &region, ModifierType type, SceneModifiedFlags flags)>;

protected:
	// lock the modifier to not perform any modification
	// this can be useful when the user is interaction with the ui elements
	// and we don't want to modify the volume
	bool _locked = false;
	bool _sliceMode = false;
	voxel::Region _sliceRegion = voxel::Region::InvalidRegion;
	core::SharedPtr<voxel::RawVolume> _sliceVolume;
	core::UUID _sliceNodeUUID;
	voxel::RawVolume *workingVolume(scenegraph::SceneGraphNode &node);
	// set to true while the brush gizmo is being interacted with
	// prevents the action button from committing intermediate transforms
	bool _brushGizmoActive = false;

	/**
	 * timer value which indicates the next execution time in case you keep the
	 * modifier triggered (stroke / continuous brushes)
	 */
	double _nextContinuousExecution = 0;
	double _nowSeconds = 0;

	BrushRegistry _registry;
	uint64_t _operationEpoch = 1;
	int _handleViewport = -1;
	uint32_t _capturedHandle = 0;
	BrushContext _brushContext;
	BrushType _brushType = BrushType::Shape;
	core::String _toolId = "shape";

	ScriptManager _scriptManager;
	PreviewManager _previewManager;

	BrushEditSession _editSession;
	uint64_t _editSettingsRevision = 0;
	BrushApplyResult _lastApplyResult;
	const char *_operationError = nullptr;

	ModifierButton _actionExecuteButton;
	ModifierButton _deleteExecuteButton;

	ModifierRendererPtr _modifierRenderer;
	SceneManager *_sceneMgr;
	core::VarPtr _autoSelect;

	void resetPreview();
	void restoreEditOverride();
	void captureEditTargets();
	void notifySessionSample(const BrushEditTarget &target, const BrushApplyResult &result);
	BrushApplyResult applySessionSample(BrushEditTarget &target, const glm::ivec3 &sample);
	bool applySessionTargets();
	bool isStrokeEdit() const;

public:
	Modifier(SceneManager *sceneMgr, const ModifierRendererPtr &modifierRenderer);
	void setSliceMode(bool active) {
		_sliceMode = active;
	}
	void setSliceRegion(const voxel::Region &region);
	const voxel::Region &sliceRegion() const {
		return _sliceRegion;
	}
	bool isSliceMode() const {
		return _sliceMode;
	}
	bool sliceActionPressed() const {
		return _actionExecuteButton.slicePressed();
	}
	bool sliceDeletePressed() const {
		return _deleteExecuteButton.slicePressed();
	}

	/**
	 * @brief Create a Raw Volume Wrapper object while taking the selection into account
	 */
	voxel::RawVolumeWrapper createRawVolumeWrapper(voxel::RawVolume *volume) const;

	void construct() override;
	bool init() override;
	void update(double nowSeconds, const video::Camera *camera);

	/**
	 * Allow to lock the modifier to not perform any modification
	 */
	void lock();
	void unlock();
	inline bool isLocked() const {
		return _locked;
	}

	inline void setBrushGizmoActive(bool active) {
		_brushGizmoActive = active;
		_brushContext.brushGizmoActive = active;
	}
	inline bool isBrushGizmoActive() const {
		return _brushGizmoActive;
	}
	/** Whether a brush action owns the current press, excluding presses routed to a gizmo. */
	bool isBrushActionActive() const {
		return _actionExecuteButton.ownsPress() || _deleteExecuteButton.ownsPress();
	}

	math::Axis lockedAxis() const;
	void setLockedAxis(math::Axis axis, bool unlock);

	void shutdown() override;

	PreviewManager &previewManager();
	const PreviewManager &previewManager() const;
	ScriptManager &scriptManager();
	const ScriptManager &scriptManager() const;

	/**
	 * @brief Resolve pending edit-session or draft work before a renderer extraction.
	 * Finishes an active edit if brush settings changed, and reevaluates a dirty adjustable draft.
	 */
	void flushPendingBrushChanges();
	void setHighlightRegion(const voxel::Region &region, uint64_t renderRegionMillis);

	void render(voxelrender::RenderContext &renderContext, const video::Camera &camera, palette::Palette &activePalette,
				const glm::mat4 &model = glm::mat4(1.0f));

	ModifierType modifierType() const;
	ModifierType setModifierType(ModifierType type);
	// get the mask of supported modifier types for the current active brush
	ModifierType checkModifierType();

	bool isMode(ModifierType modifierType) const;

	const voxel::Voxel &cursorVoxel() const;
	virtual void setCursorVoxel(const voxel::Voxel &voxel);

	/**
	 * @brief Start the current brush gesture
	 */
	bool beginBrush();

	/**
	 * @return true for tools executed by the shared sample session.
	 */
	bool usesEditSession() const;
	bool isEditActive() const;
	uint64_t editId() const {
		return _editSession.id();
	}
	/**
	 * @param overrideType Temporary operation (e.g. Erase from the delete button), or None
	 * @param sliceAction Restrict captured targets to the active slice node
	 */
	bool beginEdit(ModifierType overrideType = ModifierType::None, bool sliceAction = false);
	bool updateEdit();
	const BrushApplyResult &lastApplyResult() const {
		return _lastApplyResult;
	}
	bool finishEdit(BrushEditFinishReason reason = BrushEditFinishReason::Release);

	/**
	 * @brief Sample describing the current cursor, resolved operation, and constraints.
	 * Evaluation uses this instead of rewriting the live BrushContext.
	 */
	BrushSample currentEditSample() const;
	/**
	 * @brief Evaluate one sample on one target. Does not notify the scene or record history.
	 */
	/** Execute an explicit request without selecting a tool or changing preferences. */
	BrushApplyResult executeRequestFactory(const core::Function<core::SharedPtr<Brush>()> &create,
										   const BrushContext &input, const core::Buffer<core::UUID> &targets);
	template<class T>
	BrushApplyResult executeRequest(const T &tool, const BrushContext &input, const core::Buffer<core::UUID> &targets) {
		return executeRequestFactory([&tool]() { return core::SharedPtr<Brush>(core::make_shared<T>(tool)); }, input,
									 targets);
	}
	BrushApplyResult evaluateSample(const BrushSample &sample, BrushApplyTarget &target, bool preview = false);

	/** Shared entry point for panel and viewport parameter edits. */
	BrushApplyResult invalidParameters();
	template<class T, class Parameters>
	BrushApplyResult updateParameters(const core::String &id, const Parameters &parameters) {
		T &value = tool<T>(id);
		if (currentBrush() != &value || !value.setParameters(parameters)) {
			return invalidParameters();
		}
		return parametersChanged();
	}
	BrushApplyResult applyDraft();
	void cancelDraft();
	const char *operationError() const {
		return _operationError;
	}
	bool hasDraft() const {
		return _registry.activeDraft() != nullptr;
	}
	bool isAdjustable() const {
		return _registry.draft(currentBrush()) != nullptr;
	}
	BrushApplyResult parametersChanged();
	const scenegraph::SceneGraph &renderSceneGraph();
	void brushHandles(core::DynamicArray<BrushGizmoState> &output);
	bool editBrushHandle(const BrushHandleEvent &event);

	void preExecuteBrush(const voxel::RawVolume *volume);

	/**
	 * @brief Execute the brush operation on the given node volume
	 * @param preview When true, stroke path / blend visited state is not updated
	 */
	bool executeBrush(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, ModifierType modifierType,
					  const voxel::Voxel &voxel, const ModifiedRegionCallback &callback = {}, bool preview = false);
public:
	/**
	 * @brief Apply the current brush to one node volume
	 * @param[out,in] node The model node to modify
	 * @param callback Called for every region that was modified for the current active modifier.
	 * @note @c beginBrush() and @c endBrush() must be called before and after this method
	 */
	bool execute(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
				 const ModifiedRegionCallback &callback = {});
	void endBrush();
	/**
	 * @brief Publish a valid draft or finish the active edit session.
	 */
	void commit();
	/**
	 * @brief Publish the current draft without switching tools, then reactivate the brush.
	 */
	void brushApply();
	void abort();
	/**
	 * @brief Actions could get aborted by some external action like hitting esc
	 */
	bool aborted() const;

	BrushType setBrushType(BrushType type);
	bool setTool(const core::String &id);
	template<class T>
	T &tool(const core::String &id) {
		return _registry.get<T>(id);
	}
	template<class T>
	const T &tool(const core::String &id) const {
		return _registry.get<T>(id);
	}
	const core::String &toolId() const {
		return _toolId;
	}
	BrushRegistry &registry() {
		return _registry;
	}
	const BrushRegistry &registry() const {
		return _registry;
	}
	BrushType brushType() const;

	const Brush *currentBrush() const;
	Brush *currentBrush();
	const AABBBrush *currentAABBBrush() const;
	AABBBrush *currentAABBBrush();
	voxel::Region calcBrushRegion();

	/** Built-in accessors return the same instances owned by the registry. */
	ShapeBrush &shapeBrush();
	TextBrush &textBrush();
	LineBrush &lineBrush();
	StampBrush &stampBrush();
	PlaneBrush &planeBrush();
	PaintBrush &paintBrush();
	SelectBrush &selectBrush();
	TextureBrush &textureBrush();
	NormalBrush &normalBrush();
	ExtrudeBrush &extrudeBrush();
	TransformBrush &transformBrush();
	SculptBrush &sculptBrush();
	RulerBrush &rulerBrush();
	bool updateBoxRegion(const voxel::Region &region);
	const BrushContext &brushContext() const;

	/**
	 * @sa needsAdditionalAction()
	 */
	void executeAdditionalAction();
	/**
	 * @return @c true if the current brush needs another gesture step
	 * (for example AABB depth after spanning a plane)
	 * @sa executeAdditionalAction()
	 */
	bool needsAdditionalAction();

	/**
	 * @brief Some modifier operate on already existing voxels
	 * @note This has an influence on what the cursor voxel is - either an empty voxel - or an existing one
	 */
	bool modifierTypeRequiresExistingVoxel() const;

	/**
	 * @param[in] pos The position inside the volume - given in absolute world coordinates
	 * @param[in] face The voxel::FaceNames values where the trace hits an existing voxel
	 */
	void setCursorPosition(const glm::ivec3 &pos, voxel::FaceNames face);
	const glm::ivec3 &cursorPosition() const;
	glm::ivec3 currentCursorPosition();

	const glm::ivec3 &referencePosition() const;
	virtual void setReferencePosition(const glm::ivec3 &pos);

	const voxel::Voxel &hitCursorVoxel() const;
	void setHitCursorVoxel(const voxel::Voxel &);
	void setVoxelAtCursor(const voxel::Voxel &voxel);
	void setNormalColorIndex(uint8_t paletteIndex);
	uint8_t normalColorIndex() const;

	voxel::FaceNames cursorFace() const;

	void setGridResolution(const glm::ivec3 &gridSize);
	void setGridResolution(int gridSize);
	const glm::ivec3 &gridResolution() const;

	// TODO: only used internally or for tests
	bool autoSelect() const;
	void setAutoSelect(bool enable);

	void onSceneChange();
	void reset();
};

inline uint8_t Modifier::normalColorIndex() const {
	return _brushContext.normalIndex;
}

inline void Modifier::setNormalColorIndex(uint8_t paletteIndex) {
	if (_brushContext.normalIndex != paletteIndex) {
		_brushContext.normalIndex = paletteIndex;
		if (Brush *brush = _registry.find("normal")) {
			brush->markDirty();
		}
	}
}

inline math::Axis Modifier::lockedAxis() const {
	return _brushContext.lockedAxis;
}

inline const BrushContext &Modifier::brushContext() const {
	return _brushContext;
}

inline BrushType Modifier::brushType() const {
	return _brushType;
}

inline TextBrush &Modifier::textBrush() {
	return _registry.get<TextBrush>("text");
}

inline LineBrush &Modifier::lineBrush() {
	return _registry.get<LineBrush>("line");
}

inline ShapeBrush &Modifier::shapeBrush() {
	return _registry.get<ShapeBrush>("shape");
}

inline StampBrush &Modifier::stampBrush() {
	return _registry.get<StampBrush>("stamp");
}

inline PlaneBrush &Modifier::planeBrush() {
	return _registry.get<PlaneBrush>("plane");
}

inline PaintBrush &Modifier::paintBrush() {
	return _registry.get<PaintBrush>("paint");
}

inline NormalBrush &Modifier::normalBrush() {
	return _registry.get<NormalBrush>("normal");
}

inline SelectBrush &Modifier::selectBrush() {
	return _registry.get<SelectBrush>("select");
}

inline TextureBrush &Modifier::textureBrush() {
	return _registry.get<TextureBrush>("texture");
}

inline ExtrudeBrush &Modifier::extrudeBrush() {
	return _registry.get<ExtrudeBrush>("extrude");
}

inline TransformBrush &Modifier::transformBrush() {
	return _registry.get<TransformBrush>("transform");
}

inline SculptBrush &Modifier::sculptBrush() {
	return _registry.get<SculptBrush>("sculpt");
}

inline RulerBrush &Modifier::rulerBrush() {
	return _registry.get<RulerBrush>("ruler");
}

inline const glm::ivec3 &Modifier::gridResolution() const {
	return _brushContext.gridResolution;
}

inline const voxel::Voxel &Modifier::hitCursorVoxel() const {
	return _brushContext.hitCursorVoxel;
}

inline const glm::ivec3 &Modifier::referencePosition() const {
	return _brushContext.referencePos;
}

inline ModifierType Modifier::modifierType() const {
	return _brushContext.modifierType;
}

inline bool Modifier::isMode(ModifierType modifierType) const {
	return (_brushContext.modifierType & modifierType) != ModifierType::None;
}

inline bool Modifier::aborted() const {
	if (const Brush *brush = currentBrush()) {
		return brush->aborted(_brushContext);
	}
	return false;
}

inline void Modifier::setCursorPosition(const glm::ivec3 &pos, voxel::FaceNames face) {
	if ((_brushContext.cursorPosition != pos || _brushContext.cursorFace != face) && currentBrush() &&
		!isAdjustable()) {
		currentBrush()->markDirty();
	}
	_brushContext.cursorPosition = pos;
	_brushContext.cursorFace = face;
}

inline voxel::FaceNames Modifier::cursorFace() const {
	return _brushContext.cursorFace;
}

inline void Modifier::setCursorVoxel(const voxel::Voxel &voxel) {
	if (voxel::isAir(voxel.getMaterial())) {
		return;
	}
	if (!(_brushContext.cursorVoxel == voxel)) {
		finishEdit(BrushEditFinishReason::SettingsChange);
	}
	_brushContext.cursorVoxel = voxel;
}

inline const voxel::Voxel &Modifier::cursorVoxel() const {
	return _brushContext.cursorVoxel;
}

inline const glm::ivec3 &Modifier::cursorPosition() const {
	return _brushContext.cursorPosition;
}

inline PreviewManager &Modifier::previewManager() {
	return _previewManager;
}

inline const PreviewManager &Modifier::previewManager() const {
	return _previewManager;
}

inline ScriptManager &Modifier::scriptManager() {
	return _scriptManager;
}

inline const ScriptManager &Modifier::scriptManager() const {
	return _scriptManager;
}

inline bool Modifier::autoSelect() const {
	return _autoSelect->boolVal();
}

inline void Modifier::setAutoSelect(bool enable) {
	_autoSelect->setVal(enable);
}

} // namespace voxedit
