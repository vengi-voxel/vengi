/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "gesture/BoxGesture.h"
#include "gesture/StrokeGesture.h"
#include "voxel/Face.h"
#include "voxel/Region.h"

namespace voxedit {

/**
 * @brief Flags controlling how AABB brushes span their region
 */
enum BrushFlags : uint32_t {
	/**
	 * Standard AABB mode - first click sets one corner, second click sets opposite corner.
	 * The user can then drag to extend the third dimension.
	 */
	BRUSH_MODE_BOX = 0,

	/**
	 * Center mode - the AABB expands symmetrically around the first position.
	 * The cursor defines the extent in all directions from the center point.
	 */
	BRUSH_MODE_CENTER = 1,

	/**
	 * Stroke mode - continuously set voxels at each cursor position until the
	 * action button is released.
	 */
	BRUSH_MODE_STROKE = 2,

	/**
	 * Stroke without overlap - like stroke mode but skips the last voxel position.
	 */
	BRUSH_MODE_STROKE_NO_OVERLAP = 3,

	/**
	 * Starting value for derived classes to define their own custom flags.
	 * Extend your own flags by using this macro for the first shift value.
	 */
	BRUSH_MODE_CUSTOM = 4,
};

/**
 * @brief A brush that operates on an axis-aligned bounding box
 * @ingroup Brushes
 *
 * AABBBrush is a base class for brushes that work within a rectangular region.
 * It provides a two-step interaction model:
 *
 * 1. **beginBrush()** - Sets the first corner position
 * 2. **step()** - Updates the second corner as the cursor moves
 * 3. **executeSample()** - Generates voxels in the current region
 * 4. **endBrush()** - Completes the operation
 *
 * # Modes
 *
 * - **Box Mode** (default): Click to set first corner, move cursor to span AABB
 * - **Center Mode**: First position is the center, AABB grows symmetrically
 * - **Stroke Mode**: Place voxels continuously as the cursor moves while held (with radius support)
 * - **Stroke No-Overlap Mode**: Like Stroke but avoids re-placing at the same position
 *
 * # Grid Resolution
 *
 * Positions can snap to a grid resolution for precise aligned placement. This is
 * particularly useful when building structures that need to align to specific sizes.
 *
 * # Orthographic View Handling
 *
 * When in fixed orthographic side view mode, the brush automatically extends the
 * region through the entire volume in the view direction, since the user can't
 * freely specify depth in this view mode.
 *
 * @sa ShapeBrush - generates geometric shapes within the AABB
 * @sa PaintBrush - recolors voxels within the AABB
 * @sa SelectBrush - selects voxels within the AABB
 * @sa BoxGesture - captures the box anchors and constrains the depth step
 * @sa StrokeGesture - samples a continuous path between cursor positions
 */
class AABBBrush : public Brush {
private:
	using Super = Brush;

	/** Tracks the last cursor position to detect movement and trigger preview updates */
	glm::ivec3 _lastCursorPos{-100000};

protected:
	BoxGesture _boxGesture;
	voxel::Region _editedRegion = voxel::Region::InvalidRegion;
	uint32_t _mode = BRUSH_MODE_BOX; ///< User preference for box, center or stroke interaction
	int _radius = 0; ///< Radius for stroke mode (0 = single voxel dab)

	/**
	 * @brief Snap a position to the grid resolution
	 * @param[in] inPos Input position in voxel coordinates
	 * @param[in] resolution Grid resolution per axis (1 = no snapping on that axis)
	 * @return Position snapped to nearest grid point
	 */
	glm::ivec3 applyGridResolution(const glm::ivec3 &inPos, const glm::ivec3 &resolution) const;

	/**
	 * @brief Extend the brush region to span the entire volume in orthographic view
	 *
	 * In fixed orthographic side view mode, extends the region through the entire
	 * volume in the view direction since the user can't specify depth.
	 *
	 * @param[in] brushRegion The calculated brush region
	 * @param[in] volumeRegion The target volume's bounds
	 * @param[in] ctx The brush context (checks fixedOrthoSideView flag)
	 * @return Extended region, or original if not in ortho mode
	 */
	voxel::Region extendRegionInOrthoMode(const voxel::Region &brushRegion, const voxel::Region &volumeRegion,
										  const BrushContext &ctx) const;

	bool isMode(uint32_t flag) const;
	void setMode(uint32_t flag);

	void generateMirrored(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region);

	/**
	 * @brief Allows derived classes to override whether AABB spanning is enabled
	 *
	 * By default, AABB spanning happens unless stroke mode is active. Derived
	 * classes can override this to disable AABB behavior in specific cases
	 * (e.g., PaintBrush in plane mode).
	 *
	 * @return True if the brush should span an AABB when dragging
	 */
	virtual bool wantBox() const;

public:
	AABBBrush(BrushType type, ModifierType defaultModifier = ModifierType::Place,
			  ModifierType supportedModifiers = (ModifierType::Place | ModifierType::Erase | ModifierType::Override));
	virtual ~AABBBrush() = default;
	bool interpolatedStroke() const override {
		return anyStrokeMode();
	}
	AABBBrush *aabb() override {
		return this;
	}
	const AABBBrush *aabb() const override {
		return this;
	}
	void syncInput(const Brush &input) override;
	bool boxActive() const {
		return _boxGesture.active();
	}
	bool setRegion(const voxel::Region &region);
	void handles(const BrushContext &ctx, core::DynamicArray<BrushGizmoState> &output) const override;
	bool editHandle(uint32_t handleId, BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &delta,
					uint32_t operation) override;
	void construct() override;
	void onSceneChange() override;
	void reset() override;
	void resetRuntime() override;
	void update(const BrushContext &ctx, double nowSeconds) override;

	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;
	bool executeSample(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
					   const BrushContext &ctx) override;

	/**
	 * @brief Get the current effective cursor position during multi-step AABB creation
	 *
	 * During AABB spanning, the effective cursor position may differ from the actual
	 * cursor position. For example, when the second position is set, the cursor can
	 * move freely to define the third dimension, but two coordinates are locked to
	 * the second position.
	 *
	 * @param[in] ctx The brush context
	 * @return The effective cursor position for calculating the AABB
	 *
	 * @sa needsAdditionalAction() - checks if more input is needed
	 * @sa step() - updates the second position
	 */
	glm::ivec3 currentCursorPosition(const BrushContext &ctx) const override;

	voxel::Region calcRegion(const BrushContext &ctx) const override;

	/**
	 * @brief Set the first corner position of the AABB
	 *
	 * Marks the start of an AABB spanning operation. The cursor position from the
	 * context is captured as the first corner, snapped to the grid resolution.
	 *
	 * @param[in] ctx The brush context with the current cursor position
	 * @return True if the brush started successfully, false if already active
	 *
	 * @note Typically called by the host when the action button is pressed.
	 *
	 * @sa step() - sets/updates the second position
	 * @sa endBrush() - completes the operation
	 */
	bool beginBrush(const BrushContext &ctx) override;

	/**
	 * @brief Update the second corner position during AABB spanning
	 *
	 * Called continuously while the user is dragging to span the AABB. Updates the
	 * second corner position and marks the preview as dirty if it changed. Only has
	 * an effect after beginBrush() was called and while in AABB mode.
	 *
	 * @param[in] ctx The brush context with current cursor position
	 *
	 * @note Typically called by the host while dragging to capture the second corner or depth.
	 *
	 * @sa beginBrush() - initiates the AABB spanning
	 * @sa endBrush() - completes the operation
	 */
	void step(const BrushContext &ctx) override;

	void endBrush(BrushContext &ctx) override;
	void abort(BrushContext &ctx) override {
		endBrush(ctx);
	}

	/**
	 * @return True if the brush is currently active (between beginBrush() and endBrush())
	 */
	bool active() const override;

	/**
	 * @brief Check if the brush operation was aborted due to invalid state
	 * @param[in] ctx The brush context
	 * @return True if the face is invalid and no axis lock is set
	 */
	bool aborted(const BrushContext &ctx) const override;

	/**
	 * @brief Check if the user needs to perform another action to complete the AABB
	 *
	 * Returns true if the AABB currently spans only two dimensions (forming a plane)
	 * and needs the user to extend it in the third dimension. This happens when:
	 * - No radius is set (radius determines fixed size)
	 * - No axis lock is active (axis lock constrains to 2D)
	 * - Two dimensions already span more than grid resolution
	 * - One dimension equals the grid resolution (not yet extended)
	 *
	 * @param[in] ctx The brush context
	 * @return True if the user should continue dragging to extend the third dimension
	 *
	 * @sa currentCursorPosition() - gets the effective cursor position during extension
	 */
	bool needsAdditionalAction(const BrushContext &ctx) const override;

	/**
	 * @brief Enable center mode - AABB expands symmetrically around first position
	 *
	 * In center mode, the first position becomes the center of the AABB, and the
	 * cursor position defines how far the AABB extends in all directions.
	 */
	void setCenterMode();
	bool centerMode() const;

	/**
	 * @brief Enable stroke mode - place voxels continuously while the action is held
	 */
	void setStrokeMode();
	bool strokeMode() const;

	/**
	 * @brief Enable stroke mode without revisiting the last voxel position
	 */
	void setStrokeNoOverlap();
	bool strokeNoOverlap() const;

	/**
	 * @return True if either stroke mode is active
	 */
	bool anyStrokeMode() const;

	/**
	 * @return True while a stroke drag is in progress (after beginBrush, before endBrush)
	 */

	/**
	 * @brief Enable box mode (default) - span a rectangular region
	 */
	void setBoxMode();
	bool boxMode() const;

	/**
	 * @brief Set the radius for stroke mode operations
	 *
	 * When in stroke mode with a radius > 0, each placement creates a cube of
	 * voxels with the given radius around the cursor position.
	 *
	 * @param[in] radius The radius in voxels (0 = single voxel)
	 */
	void setRadius(int radius);

	/**
	 * @return The current radius, or 0 if not in stroke mode
	 */
	int radius() const;

	int previewRadius() const override;
	bool wantsContinuousExecution() const override {
		return anyStrokeMode();
	}
	bool executeOnPress() const override {
		return anyStrokeMode();
	}
};

inline int AABBBrush::radius() const {
	if (!anyStrokeMode()) {
		return 0;
	}
	return _radius;
}

inline int AABBBrush::previewRadius() const {
	if (!anyStrokeMode() || _radius <= 0) {
		return -1;
	}
	return _radius;
}

inline bool AABBBrush::centerMode() const {
	return isMode(BRUSH_MODE_CENTER);
}

inline void AABBBrush::setCenterMode() {
	markSettingsChanged();
	setMode(BRUSH_MODE_CENTER);
}

inline bool AABBBrush::anyStrokeMode() const {
	return strokeMode() || strokeNoOverlap();
}

inline bool AABBBrush::strokeMode() const {
	return isMode(BRUSH_MODE_STROKE);
}

inline void AABBBrush::setStrokeMode() {
	markSettingsChanged();
	setMode(BRUSH_MODE_STROKE);
}

inline bool AABBBrush::strokeNoOverlap() const {
	return isMode(BRUSH_MODE_STROKE_NO_OVERLAP);
}

inline void AABBBrush::setStrokeNoOverlap() {
	markSettingsChanged();
	setMode(BRUSH_MODE_STROKE_NO_OVERLAP);
}

inline bool AABBBrush::boxMode() const {
	return isMode(BRUSH_MODE_BOX);
}

inline void AABBBrush::setBoxMode() {
	markSettingsChanged();
	setMode(BRUSH_MODE_BOX);
}

} // namespace voxedit
