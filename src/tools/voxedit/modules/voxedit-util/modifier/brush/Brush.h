/**
 * @file
 * @defgroup Brushes Brushes
 * @{
 * Editor brushes.
 * @}
 */

#pragma once

#include "BrushType.h"
#include "core/DirtyState.h"
#include "core/IComponent.h"
#include "core/String.h"
#include "math/Axis.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/SceneModifiedFlags.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace scenegraph {
class SceneGraph;
}

namespace voxedit {

class ModifierVolumeWrapper;

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
	/** Grid resolution for snapping operations - voxels are placed at multiples of this value */
	int gridResolution = 1;

	/** Used for clamping the brush region to stay within the target volume boundaries */
	voxel::Region targetVolumeRegion;

	/** The position of the cursor before any clamping or brush execution was applied */
	glm::ivec3 prevCursorPosition{0};

	/** The modifier operation to perform (Place, Erase, Override, Paint, Select) */
	ModifierType modifierType = ModifierType::Place;
};

/**
 * @brief Base class for all brushes in the voxel editor
 * @ingroup Brushes
 *
 * The brush system provides different tools for placing, modifying, and selecting voxels.
 * Each brush defines its own behavior through the generate() method, while the base class
 * handles common functionality like mirroring, clamping, and the execution lifecycle.
 *
 * # Brush Lifecycle
 *
 * A typical brush operation follows this sequence:
 * 1. **beginBrush()** - Called when the user starts an action (e.g., mouse down)
 * 2. **preExecute()** - Prepare any state before execution
 * 3. **execute()** - Main execution that calls generate() for the affected regions
 * 4. **endBrush()** - Cleanup after the operation completes
 *
 * Between beginBrush() and endBrush(), update() is called each frame to handle preview
 * updates. If the user cancels, abort() is called instead of endBrush().
 *
 * # Mirroring System
 *
 * Brushes support mirroring along X, Y, or Z axes. When mirroring is enabled, execute()
 * automatically generates voxels in both the primary region and the mirrored region.
 * The mirror position is typically set to the reference position and acts as the plane
 * of symmetry.
 *
 * @sa ModifierVolumeWrapper
 * @sa BrushContext
 */
class Brush : public core::IComponent, public core::DirtyState {
protected:
	const BrushType _brushType;				///< The type of this brush (Shape, Line, Paint, etc.)
	const ModifierType _defaultModifier;	///< Default operation type if none specified
	const ModifierType _supportedModifiers; ///< Bitfield of supported ModifierTypes
	SceneModifiedFlags _sceneModifiedFlags = SceneModifiedFlags::All; ///< What scene state gets invalidated

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
	bool _brushClamping = false;

	Brush(BrushType brushType, ModifierType defaultModifier = ModifierType::Place,
		  ModifierType supportedModifiers = (ModifierType::Place | ModifierType::Erase | ModifierType::Override))
		: _brushType(brushType), _defaultModifier(defaultModifier), _supportedModifiers(supportedModifiers) {
	}

	/**
	 * @brief Generate the voxels for this brush operation
	 *
	 * This is the core method each brush must implement. It receives a region to fill
	 * and should use the ModifierVolumeWrapper to place/modify voxels. The method is
	 * called by execute() - if mirroring is enabled, it will be called multiple times
	 * with different regions.
	 *
	 * @param[in] sceneGraph The scene graph to operate on
	 * @param[in] wrapper The volume wrapper providing voxel access and undo support
	 * @param[in] ctx The brush context with cursor position, voxel type, etc.
	 * @param[in] region The region to operate in - may be invalid depending on the brush type
	 *
	 * @note When mirroring is active, this method is called once for the primary region
	 *       and once for the mirrored region with adjusted bounds
	 *
	 * @sa execute() - orchestrates the generation with mirroring support
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
	void markDirty() override {
		core::DirtyState::markDirty();
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
	 * @sa preExecute() - called before execute() to prepare state
	 * @sa execute() - performs the actual voxel modifications
	 * @sa endBrush() - called when the operation completes
	 */
	virtual bool beginBrush(const BrushContext &ctx);

	/**
	 * @brief Prepare the brush state before execution
	 *
	 * Called immediately before execute() to allow brushes to capture any state they
	 * need from the current volume. For example, the PlaneBrush caches the voxel that
	 * was hit to know which voxels to extrude.
	 *
	 * @param[in] ctx The brush context
	 * @param[in] volume The volume that is currently active - can be null if no volume loaded
	 *
	 * @sa beginBrush() - starts the brush operation
	 * @sa execute() - main execution after this preparation
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
	 * @param[in] wrapper The volume wrapper providing voxel access with undo support
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
	 * @brief Called when brush action completes successfully
	 *
	 * Perform any cleanup needed after the brush operation finishes. For continuous
	 * brushes (like the LineBrush), this might update the reference position to
	 * continue from where the last action ended.
	 *
	 * @param[out] ctx The brush context - can be modified (e.g., update referencePos)
	 *
	 * @sa beginBrush() - starts the operation
	 * @sa execute() - performs the action
	 * @sa abort() - alternative completion when user cancels
	 */
	virtual void endBrush(BrushContext &ctx);

	/**
	 * @brief Abort the brush operation
	 *
	 * Called when the user cancels the brush action (e.g., pressing Escape).
	 * Default implementation does nothing, but brushes can override to reset
	 * state differently than endBrush().
	 *
	 * @param[out] ctx The brush context
	 *
	 * @sa beginBrush() - starts the operation
	 * @sa endBrush() - normal completion path
	 */
	virtual void abort(BrushContext &ctx) {
	}

	/**
	 * @brief Reset the brush to initial state
	 *
	 * Clears all brush state including mirroring, clamping, and internal modes.
	 * Forces a recreation of the preview volume. Called when switching brushes
	 * or resetting the editor state.
	 */
	virtual void reset();

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
	 * @return Flags indicating what scene state is invalidated by this brush
	 */
	SceneModifiedFlags sceneModifiedFlags() const;

	/**
	 * @brief Calculate the region this brush will modify
	 *
	 * Each brush defines its own logic for determining the affected region.
	 * For example:
	 * - AABBBrush: region between first and second corner positions
	 * - LineBrush: bounding box containing the line from reference to cursor
	 * - StampBrush: region covered by the stamp volume
	 *
	 * This region does not include mirroring - the execute() method handles that.
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
	 * @param[in] brushClamping True to enable clamping
	 */
	void setBrushClamping(bool brushClamping);

	/**
	 * @return True if brush clamping is enabled
	 */
	bool brushClamping() const;

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
	 * across the current mirror axis and position. Used internally by execute()
	 * to generate voxels symmetrically.
	 *
	 * @param[in,out] mins Lower corner of the AABB - will be updated to mirrored position
	 * @param[in,out] maxs Upper corner of the AABB - will be updated to mirrored position
	 * @return True if mirroring is active and bounds were modified, false if no mirroring
	 *
	 * @sa setMirrorAxis() - enables mirroring
	 * @sa mirrorAxis() - queries the current mirror state
	 */
	bool getMirrorAABB(glm::ivec3 &mins, glm::ivec3 &maxs) const;

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
	 * @sa getMirrorAABB() - calculates mirrored region bounds
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
