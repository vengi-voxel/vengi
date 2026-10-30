/**
 * @file
 * @defgroup Brushes Brushes
 * @{
 * Editor brushes.
 * @}
 */

#pragma once

#include "BrushGizmo.h"
#include "BrushType.h"
#include "core/DirtyState.h"
#include "core/IComponent.h"
#include "core/String.h"
#include "core/collection/Buffer.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace scenegraph {
class SceneGraph;
}

namespace math {
class Ray;
}

namespace voxedit {

class ModifierVolumeWrapper;
class AABBBrush;

enum class BrushPreviewStyle { Footprint, Changes, Selection };

/**
 * @brief Parse ve_gridsize style values into a per-axis resolution
 *
 * Accepts a single number ("3" -> (3,3,3)) or three components
 * separated by space, colon, or comma ("1 2 3" / "1:2:3").
 * Each component is clamped to 1..64.
 */
glm::ivec3 parseGridResolution(const core::String &str);

/**
 * @brief Context information passed to brush operations containing all necessary state
 *
 * This structure contains all the information a brush needs to perform its operation,
 * including cursor position, voxel selection, view mode, and constraints. It acts as
 * the parameter object passed through the brush lifecycle.
 */
struct BrushContext {
	/** The voxel that should get placed by the brush */
	voxel::Voxel cursorVoxel;
	/** The existing voxel under the cursor (before the hit face) */
	voxel::Voxel hitCursorVoxel;
	/** The voxel where the cursor is - can be air */
	voxel::Voxel voxelAtCursor;

	/** Reference/start position for multi-point operations (e.g., line start, AABB first corner) */
	glm::ivec3 referencePos{0};
	/** Current cursor position in voxel coordinates */
	glm::ivec3 cursorPosition{0};
	/** The face where the raycast hit - determines placement direction */
	voxel::FaceNames cursorFace = voxel::FaceNames::Max;
	/** Axis lock constraint for 2D operations (e.g., drawing on a plane) */
	math::Axis lockedAxis = math::Axis::None;

	/**
	 * True when in orthographic side view mode. Brushes that span an AABB behave
	 * differently since you can't freely span all three dimensions in this view.
	 */
	bool fixedOrthoSideView = false;
	bool normalPaint = false;
	uint8_t normalIndex = NO_NORMAL;
	/** Grid resolution for snapping operations - voxels are placed at multiples of this value per axis */
	glm::ivec3 gridResolution{1};

	/** Used for clamping the brush region to stay within the target volume boundaries */
	voxel::Region targetVolumeRegion;
	/** Explicit operation bounds supplied by a request, or InvalidRegion to use gesture geometry. */
	voxel::Region evaluatedRegion = voxel::Region::InvalidRegion;

	/** The position of the cursor before any clamping or brush execution was applied */
	glm::ivec3 prevCursorPosition{0};

	/** True while viewport handle interaction owns or blocks brush input */
	bool brushGizmoActive = false;

	/**
	 * True when evaluating a private preview rather than a committed sample.
	 * The host supplies isolated evaluator runtime; algorithms may update that
	 * private runtime without affecting the active stroke or its random sequence.
	 */
	bool preview = false;

	/** The modifier operation to perform (Place, Erase, Override, Paint, Select) */
	ModifierType modifierType = ModifierType::Place;
};

/**
 * @brief Base class for brushes in the voxel editor
 * @ingroup Brushes
 *
 * Each brush defines its algorithm through generate(). The base class provides
 * mirroring, clamping settings, and default execution helpers. Modifier owns
 * target resolution, input capture, preview lifetime, publication, and undo.
 *
 * A registered tool holds persistent settings and input geometry. For ordinary
 * edits, the host calls beginBrush(), creates an evaluator copy for each target,
 * and calls preExecute() followed by executeSample() for each sample. endBrush()
 * finishes the input gesture; resetRuntime() discards transient state. Adjustable
 * edits use a registered draft controller to evaluate captured operation parameters.
 *
 * Preview evaluates a private copy of the target runtime, so it must not change
 * committed voxels, selection, palettes, random state, or undo history.
 *
 * Brushes support mirroring along X, Y, or Z. The mirror position defines the
 * plane of symmetry and is normally captured from the reference position.
 * Execution helpers generate the primary and mirrored regions as needed.
 *
 * @sa ModifierVolumeWrapper
 * @sa BrushContext
 */
class Brush : public core::IComponent, public core::DirtyState {
private:
	uint64_t _settingsRevision = 0;

protected:
	/** Advance the settings revision when a persistent preference changes. */
	void markSettingsChanged() {
		++_settingsRevision;
		markDirty();
	}
	const BrushType _brushType;				///< The type of this brush (Shape, Line, Paint, etc.)
	const ModifierType _defaultModifier;	///< Default operation type if none specified
	const ModifierType _supportedModifiers; ///< Bitfield of supported ModifierTypes

	glm::ivec3 _referencePosition{0}; ///< Cached reference position for mirror commands

	core::String _errorReason; ///< Error message shown in UI when brush can't be used

	/**
	 * The mirror position defines the plane of symmetry. It's based on the reference
	 * position whenever the mirror axis is set.
	 */
	glm::ivec3 _mirrorPos{0};
	math::Axis _mirrorAxis = math::Axis::None; ///< Active mirror axis, or None if disabled
	void toggleMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos);

	/**
	 * Controls whether the brush region is automatically clamped to stay within the
	 * target volume boundaries. When enabled, the cursor position may be adjusted
	 * to prevent the brush from extending outside the volume.
	 */
	bool _clampToVolume = false;

	Brush(const Brush &other)
		: core::IComponent(), core::DirtyState(other), _brushType(other._brushType),
		  _defaultModifier(other._defaultModifier), _supportedModifiers(other._supportedModifiers),
		  _referencePosition(other._referencePosition), _errorReason(other._errorReason), _mirrorPos(other._mirrorPos),
		  _mirrorAxis(other._mirrorAxis), _clampToVolume(other._clampToVolume) {
	}

	Brush(BrushType brushType, ModifierType defaultModifier = ModifierType::Place,
		  ModifierType supportedModifiers = (ModifierType::Place | ModifierType::Erase | ModifierType::Override))
		: _brushType(brushType), _defaultModifier(defaultModifier), _supportedModifiers(supportedModifiers) {
	}

	/**
	 * @brief Generate the voxels for this brush operation
	 *
	 * This is the core method each brush must implement. It receives a region to evaluate
	 * and should use the ModifierVolumeWrapper to place/modify voxels. The method is
	 * called by the execution helpers; with mirroring enabled it may be called multiple times
	 * with different regions.
	 *
	 * @param[in] sceneGraph The scene graph to operate on
	 * @param[in] wrapper The volume wrapper providing constrained voxel access and change tracking
	 * @param[in] ctx The brush context with cursor position, voxel type, etc.
	 * @param[in] region The region to operate in - may be invalid depending on the brush type
	 *
	 * @note When mirroring is active, this method is called once for the primary region
	 *       and once for the mirrored region with adjusted bounds
	 *
	 * @sa executeSample() - evaluates a sample with the brush execution policy
	 * @sa calcRegion() - defines the region passed to this method
	 */
	virtual void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region) = 0;

	/**
	 * @brief Set an error message to be displayed in the UI
	 *
	 * This can be used to communicate to the user why a brush can't be used,
	 * such as missing configuration or invalid state. The error appears as a
	 * tooltip at the cursor.
	 */
	void setErrorReason(const core::String &reason) {
		_errorReason = reason;
	}

public:
	/** Revision used by the host to resolve settings changes before the next sample. */
	uint64_t settingsRevision() const {
		return _settingsRevision;
	}

	/** Return the standard AABB interaction adapter, or nullptr for other tools. */
	virtual AABBBrush *aabb() {
		return nullptr;
	}
	/** Const overload of aabb(). */
	virtual const AABBBrush *aabb() const {
		return nullptr;
	}

	void markDirty() override {
		core::DirtyState::markDirty();
	}

	/**
	 * @brief Report separate changed regions provided by the brush algorithm
	 * @param[out] regions Buffer to fill with changed regions
	 * @return true if the brush supplies its own region list
	 */
	virtual bool dirtyRegions(core::Buffer<voxel::Region> &regions) const {
		return false;
	}

	/**
	 * @brief Get the error reason if the brush is not usable - this can be shown as
	 * tooltip for the cursor in the viewport
	 */
	const core::String &errorReason() const {
		return _errorReason;
	}

	/**
	 * @brief Start the brush action
	 *
	 * Called when the user initiates a brush operation (typically on mouse down).
	 * Brushes can use this to capture initial state, like the first corner of an AABB.
	 *
	 * @param[in] ctx The current brush context
	 * @return true if the brush started successfully, false if already active or invalid state
	 *
	 * @sa preExecute() - called before executeSample() to prepare state
	 * @sa executeSample() - evaluates a sample with the brush execution policy
	 * @sa endBrush() - called when the operation completes
	 */
	virtual bool beginBrush(const BrushContext &ctx);

	/**
	 * @brief Prepare the brush state before execution
	 *
	 * Called before executeSample() to allow brushes to capture any state they
	 * need from the current volume, such as selected bounds or cached source geometry.
	 *
	 * @param[in] ctx The brush context
	 * @param[in] volume The volume that is currently active - can be null if no volume loaded
	 *
	 * @sa beginBrush() - starts the brush operation
	 * @sa executeSample() - evaluates a sample after this preparation
	 * @sa endBrush() - cleanup after execution
	 */
	virtual void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume);

	/**
	 * @brief Execute the brush action on the given volume
	 *
	 * This is the main method that performs the brush operation. It calculates the
	 * affected region(s), handles mirroring automatically, and calls generate() for
	 * each region that needs voxels placed.
	 *
	 * The execution flow:
	 * 1. Calculate the primary region via calcRegion()
	 * 2. If mirroring is disabled: call generate() once for the primary region
	 * 3. If mirroring is enabled: calculate the mirrored region and call generate()
	 *    for both regions (or a combined region if they overlap)
	 *
	 * @param[in] sceneGraph The scene graph to operate on
	 * @param[in] wrapper The volume wrapper providing constrained voxel access and change tracking
	 * @param[in] ctx The brush context with all necessary parameters
	 * @return true if the brush action was successful
	 *
	 * @sa beginBrush() - initiates the operation
	 * @sa preExecute() - prepares state before this method
	 * @sa generate() - the actual voxel placement logic
	 * @sa endBrush() - cleanup after execution
	 */
	virtual bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx);

	/**
	 * @brief Evaluate one sample without advancing the host's stroke sampler
	 *
	 * The default delegates to execute(). AABBBrush evaluates the current footprint;
	 * the host handles path interpolation, target iteration, and grouped undo.
	 * Preview uses the same entry point with an isolated evaluator and target.
	 */
	virtual bool executeSample(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
							   const BrushContext &ctx) {
		return execute(sceneGraph, wrapper, ctx);
	}

	/**
	 * @brief Finish the current input gesture
	 *
	 * Perform any cleanup needed after the brush operation finishes. For continuous
	 * brushes (like the LineBrush), this might update the reference position to
	 * continue from where the last action ended.
	 *
	 * @param[out] ctx The brush context - can be modified (e.g., update referencePos)
	 *
	 * @sa beginBrush() - starts the operation
	 * @sa execute() - performs the action
	 * @note The host records history and may also finish a gesture after interruption.
	 */
	virtual void endBrush(BrushContext &ctx);

	/**
	 * @brief Discard tool-local gesture state when the host has no active edit session
	 *
	 * Active sessions and adjustable drafts are resolved by Modifier. The default
	 * implementation does nothing; it does not restore scene data or record undo.
	 * @param[in,out] ctx The brush context, which may be updated during cleanup
	 */
	virtual void abort(BrushContext &ctx) {
	}

	/**
	 * @brief Called when the scene has changed externally (e.g. via undo/redo)
	 *
	 * Brushes should override this to invalidate any cached state that depends
	 * on the volume data (cached regions and prepared input)
	 * without resetting user preferences (mirror settings, brush configuration, etc.).
	 *
	 * The default implementation just marks the preview as dirty.
	 */
	virtual void onSceneChange();

	/**
	 * @brief Called when this brush becomes the active brush.
	 */
	virtual void onActivated() {
	}

	/** Choose footprint, changed-voxel, or selection rendering for the evaluated preview. */
	virtual BrushPreviewStyle previewStyle() const {
		return BrushPreviewStyle::Footprint;
	}
	/** Whether preview evaluation needs a copy of existing voxels rather than an empty target. */
	virtual bool previewNeedsExistingVolume() const {
		return false;
	}
	/** Number of source voxels around the output footprint needed for neighbor reads. */
	virtual int previewPadding() const {
		return 0;
	}
	/** Output bounds for preview allocation; tools may inspect source geometry without changing their runtime. */
	virtual voxel::Region previewRegion(const BrushContext &ctx, const voxel::RawVolume *volume) const {
		return calcRegion(ctx);
	}
	/** Override surface picking while dragging, in model voxel coordinates. */
	virtual bool projectCursor(const math::Ray &ray, glm::ivec3 &position) const {
		return false;
	}
	/** Map the requested operation to the write policy enforced by ModifierVolumeWrapper. */
	virtual ModifierType volumeModifier(ModifierType requested) const {
		return requested;
	}
	/** Whether the host may apply its automatic selection policy to placed voxels. */
	virtual bool autoSelectPlacement() const {
		return !managesOwnSelection();
	}
	/** Whether the tool contributes interaction and presentation without publishing voxel edits. */
	virtual bool readOnly() const {
		return false;
	}

	/** Copy current input geometry into an evaluator without overwriting its per-target runtime. */
	virtual void syncInput(const Brush &input) {
	}
	/** Initialize operation-local randomness; preview copies the evaluator without advancing it. */
	virtual void seedRuntime(uint32_t seed) {
	}

	/** Whether to render an outline instead of evaluating a filled preview volume. */
	virtual bool isSimplePreview() const {
		return false;
	}

	/**
	 * @brief Radius for the shared surface radius outline preview, or -1 to hide it.
	 *
	 * Brushes that place/affect a circular footprint around the cursor (stroke AABB
	 * radius, sculpt brush radius, ...) should return >= 0 while that mode is active.
	 */
	virtual int previewRadius() const {
		return -1;
	}

	/**
	 * @brief When true, skip filled cube/volume preview and keep the red face cursor.
	 *
	 * Single-voxel stroke (radius 0) would otherwise draw a 1x1x1 cube that hides
	 * the face-aligned cursor.
	 */
	virtual bool preferFaceCursorPreview() const {
		return false;
	}

	/**
	 * @brief Reset the brush to initial state
	 *
	 * Clears all brush state including mirroring, clamping, and internal modes.
	 * Forces a recreation of the preview volume. Called when resetting the editor state.
	 */
	virtual void reset();

	/**
	 * @brief Discard transient gesture and evaluation state while retaining settings
	 * Used when an edit finishes or the active tool changes. The default marks the
	 * preview dirty; overrides clear their own input geometry and runtime caches.
	 */
	virtual void resetRuntime();

	/**
	 * @brief Update the brush state each frame
	 *
	 * Called continuously while the brush is active. Brushes should check if their
	 * state has changed (cursor moved, voxel changed, etc.) and call markDirty() if
	 * the preview needs to be regenerated.
	 *
	 * @param[in] ctx The current brush context
	 * @param[in] nowSeconds Current time in seconds for time-based effects
	 *
	 * @sa markDirty() - marks the preview as needing regeneration
	 */
	virtual void update(const BrushContext &ctx, double nowSeconds);
	/**
	 * @return The type of the brush as string (for UI display)
	 * @sa type()
	 */
	core::String name() const;

	/**
	 * @return The BrushType enum value
	 * @sa name()
	 */
	BrushType type() const;

	/**
	 * @return true if this brush directly manages voxel selection flags (FlagOutline).
	 * When true, growSelectionToNewVoxels/autoSelectNewVoxels are skipped after execution
	 * to avoid overwriting the brush's own selection logic.
	 */
	virtual bool managesOwnSelection() const {
		return false;
	}

	/** Whether the host should fill gaps between cursor samples using the shared stroke sampler. */
	virtual bool interpolatedStroke() const {
		return false;
	}

	/**
	 * @return true to evaluate samples while the action button is held, rather than
	 * waiting for release. Used for painting and other continuous interactions.
	 */
	virtual bool wantsContinuousExecution() const {
		return false;
	}

	/** Whether the first sample is applied immediately when the action button is pressed. */
	virtual bool executeOnPress() const {
		return false;
	}

	/** Capture an intermediate gesture step, such as the second corner before defining depth. */
	virtual void step(const BrushContext &ctx) {
	}

	/** Whether the current gesture needs another action, such as defining box depth. */
	virtual bool needsAdditionalAction(const BrushContext &ctx) const {
		return false;
	}

	/** Whether the current gesture state prevents further evaluation. */
	virtual bool aborted(const BrushContext &ctx) const {
		return false;
	}

	/** Cursor position constrained by the active gesture. */
	virtual glm::ivec3 currentCursorPosition(const BrushContext &ctx) const {
		return ctx.cursorPosition;
	}

	/**
	 * @brief Calculate the region this brush will modify
	 *
	 * Each brush defines its own logic for determining the affected region.
	 * For example:
	 * - AABBBrush: region between first and second corner positions
	 * - LineBrush: bounding box containing the line from reference to cursor
	 * - StampBrush: region covered by the stamp volume
	 *
	 * This region does not include mirroring - the execution helpers handle that.
	 *
	 * @param[in] ctx The current brush context
	 * @return The region that will be affected by the brush operation
	 *
	 * @note The returned region might be invalid for some brushes depending on state
	 */
	virtual voxel::Region calcRegion(const BrushContext &ctx) const = 0;

	/**
	 * @brief Adjust modifier type based on what this brush supports
	 *
	 * Brushes can restrict which modifier types they support. For example, the
	 * PaintBrush only supports ModifierType::Paint. If an unsupported type is
	 * requested, this returns the default modifier for the brush.
	 *
	 * @param[in] type The requested modifier type (or None to get default)
	 * @return The actual modifier type to use, guaranteed to be supported
	 */
	ModifierType modifierType(ModifierType type = ModifierType::None) const;

	/**
	 * @brief Enable or disable automatic brush region clamping
	 *
	 * When enabled, the brush region is automatically adjusted to stay within
	 * the target volume boundaries. The cursor position may be modified to
	 * achieve this.
	 *
	 * @param[in] clampToVolume True to enable clamping
	 */
	void setClampToVolume(bool clampToVolume);

	/**
	 * @return True if brush clamping is enabled
	 */
	bool clampToVolume() const;

	/**
	 * @brief Determine whether the brush should be rendered
	 *
	 * Controls preview rendering. Most brushes are always active, but some
	 * (like StampBrush) require configuration before they can be used.
	 *
	 * @return True if the brush preview should be rendered
	 */
	virtual bool active() const;

	void construct() override;
	bool init() override;
	void shutdown() override;

	/**
	 * @brief Calculate the mirrored AABB coordinates
	 *
	 * Given an AABB, calculates where it would be positioned when mirrored
	 * across the current mirror axis and position. Used by the execution helpers
	 * to generate voxels symmetrically.
	 *
	 * @param[in,out] mins Lower corner of the AABB - will be updated to mirrored position
	 * @param[in,out] maxs Upper corner of the AABB - will be updated to mirrored position
	 * @return True if mirroring is active and bounds were modified, false if no mirroring
	 *
	 * @sa setMirrorAxis() - enables mirroring
	 * @sa mirrorAxis() - queries the current mirror state
	 */
	bool getMirrorBox(glm::ivec3 &mins, glm::ivec3 &maxs) const;

	/**
	 * @brief Set or change the mirror axis and position
	 *
	 * Enables mirroring for this brush. All voxel operations will be duplicated
	 * symmetrically across the mirror plane defined by the axis and position.
	 *
	 * @param[in] axis The axis to mirror across (X, Y, Z, or None to disable)
	 * @param[in] mirrorPos The position of the mirror plane
	 * @return True if the mirror state changed
	 *
	 * @sa getMirrorBox() - calculates mirrored region bounds
	 * @sa mirrorAxis() - queries current axis
	 * @sa mirrorPos() - queries current position
	 */
	bool setMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos);

	/**
	 * @return The currently active mirror axis, or Axis::None if mirroring is disabled
	 */
	math::Axis mirrorAxis() const;

	/**
	 * @return The position defining the mirror plane
	 */
	const glm::ivec3 &mirrorPos() const;

	/**
	 * @brief Append the brush's viewport handles and overlays
	 *
	 * Populate each BrushGizmoState with a transform, supported operations, and
	 * optional bounds or overlay geometry. Editable handles need stable IDs within
	 * the operation. The host assigns operation ownership and routes viewport input.
	 *
	 * @param[in] ctx The current brush context
	 * @param[out] output Collection to append handle descriptions to
	 * @sa editHandle()
	 */
	virtual void handles(const BrushContext &ctx, core::DynamicArray<BrushGizmoState> &output) const;

	/**
	 * @brief Apply a viewport handle change to the corresponding tool parameters
	 *
	 * The host validates operation, handle, and viewport ownership before calling
	 * this method. Update the same parameters used by the settings panel; the host
	 * refreshes the preview and owns publication and undo.
	 *
	 * @param[in] handleId Stable ID from handles()
	 * @param[in,out] ctx The brush context
	 * @param[in] matrix Manipulated transform in model voxel coordinates
	 * @param[in] deltaMatrix Change to the transform in the same coordinate system
	 * @param[in] operation Active BrushGizmoOperation flags
	 * @return true if the tool parameters changed
	 */
	virtual bool editHandle(uint32_t handleId, BrushContext &ctx, const glm::mat4 &matrix,
								const glm::mat4 &deltaMatrix, uint32_t operation);
};

inline const glm::ivec3 &Brush::mirrorPos() const {
	return _mirrorPos;
}

inline math::Axis Brush::mirrorAxis() const {
	return _mirrorAxis;
}

inline BrushType Brush::type() const {
	return _brushType;
}

inline core::String Brush::name() const {
	return BrushTypeStr[(int)_brushType];
}

} // namespace voxedit
