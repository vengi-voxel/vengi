/**
 * @file
 */

#pragma once

#include "Brush.h"
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
	BRUSH_MODE_AABB = 0,

	/**
	 * Center mode - the AABB expands symmetrically around the first position.
	 * The cursor defines the extent in all directions from the center point.
	 */
	BRUSH_MODE_CENTER = 1,

	/**
	 * Single mode - continuously set voxels at each cursor position until the
	 * action button is released. Useful for "painting" multiple voxels.
	 */
	BRUSH_MODE_SINGLE = 2,

	/**
	 * Single move mode - like SINGLE but doesn't overwrite the last voxel position.
	 * Prevents repeated placement at the same location.
	 */
	BRUSH_MODE_SINGLE_MOVE = 3,

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
 * 3. **execute()** - Generates voxels in the final region
 * 4. **endBrush()** - Completes the operation
 *
 * # Modes
 *
 * - **AABB Mode** (default): Click to set first corner, move cursor to span AABB
 * - **Center Mode**: First position is the center, AABB grows symmetrically
 * - **Single Mode**: Place voxels continuously as cursor moves (with radius support)
 * - **Single Move Mode**: Like Single but avoids re-placing at same position
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
 */
class AABBBrush : public Brush {
private:
	using Super = Brush;

	/** Tracks the last cursor position to detect movement and trigger preview updates */
	glm::ivec3 _lastCursorPos{-100000};

protected:
	/**
	 * True if currently spanning an AABB (between beginBrush() and endBrush()).
	 * The first position of the AABB is now set.
	 */
	bool _aabbMode = false;

	/**
	 * True if the AABB has both valid mins and maxs positions, but the maxs
	 * can still be modified by calling step(). This represents the intermediate
	 * state where two corners are set but the third dimension can still be adjusted.
	 */
	bool _secondPosValid = false;

	/**
	 * When spanning an AABB, this stores which face was initially hit.
	 * This determines the primary plane being spanned and affects how the third
	 * dimension is extended.
	 */
	voxel::FaceNames _aabbFace = voxel::FaceNames::Max;

	uint32_t _mode = BRUSH_MODE_AABB; ///< Current brush mode flags
	int _radius = 0;				  ///< Radius for single mode (0 = single voxel)

	/**
	 * The first corner position of the AABB, set by beginBrush()
	 */
	glm::ivec3 _aabbFirstPos{0};

	/**
	 * The second corner position of the AABB, updated by step().
	 * Together with the first position, this defines two dimensions of the AABB.
	 * The cursor can then be used to span the third dimension.
	 */
	glm::ivec3 _aabbSecondPos{0};

	/**
	 * @brief Snap a position to the grid resolution
	 * @param[in] inPos Input position in voxel coordinates
	 * @param[in] resolution Grid resolution (1 = no snapping)
	 * @return Position snapped to nearest grid point
	 */
	glm::ivec3 applyGridResolution(const glm::ivec3 &inPos, int resolution) const;

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

	/**
	 * @brief Allows derived classes to override whether AABB spanning is enabled
	 *
	 * By default, AABB spanning happens unless single mode is active. Derived
	 * classes can override this to disable AABB behavior in specific cases
	 * (e.g., PaintBrush in plane mode).
	 *
	 * @return True if the brush should span an AABB when dragging
	 */
	virtual bool wantAABB() const;

public:
	AABBBrush(BrushType type, ModifierType defaultModifier = ModifierType::Place,
			  ModifierType supportedModifiers = (ModifierType::Place | ModifierType::Erase | ModifierType::Override));
	virtual ~AABBBrush() = default;
	void construct() override;
	void reset() override;
	void update(const BrushContext &ctx, double nowSeconds) override;

	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;

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
	glm::ivec3 currentCursorPosition(const BrushContext &ctx) const;

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
	 * @note This is typically called on mouse down in input handlers
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
	 * @note This is typically called on mouse move in input handlers
	 *
	 * @sa beginBrush() - initiates the AABB spanning
	 * @sa endBrush() - completes the operation
	 */
	void step(const BrushContext &ctx);

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
	bool aborted(const BrushContext &ctx) const;

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
	virtual bool needsAdditionalAction(const BrushContext &ctx) const;

	/**
	 * @brief Enable center mode - AABB expands symmetrically around first position
	 *
	 * In center mode, the first position becomes the center of the AABB, and the
	 * cursor position defines how far the AABB extends in all directions.
	 */
	void setCenterMode();
	bool centerMode() const;

	/**
	 * @brief Enable single mode - place voxels continuously
	 *
	 * In single mode, voxels are placed at each cursor position as long as the
	 * action button is held. Useful for "painting" voxels.
	 */
	void setSingleMode();
	bool singleMode() const;

	/**
	 * @brief Enable single move mode - like single mode but avoids repeating last position
	 *
	 * Similar to single mode but prevents placing the same voxel multiple times
	 * at the same location. The scene trace is not reset between placements.
	 */
	void setSingleModeMove();
	bool singleModeMove() const;

	/**
	 * @return True if either single mode or single move mode is active
	 */
	bool anySingleMode() const;

	/**
	 * @brief Enable AABB mode (default) - span rectangular region
	 */
	void setAABBMode();
	bool aabbMode() const;

	/**
	 * @brief Set the radius for single mode operations
	 *
	 * When in single mode with a radius > 0, each placement creates a cube of
	 * voxels with the given radius around the cursor position.
	 *
	 * @param[in] radius The radius in voxels (0 = single voxel)
	 */
	void setRadius(int radius);

	/**
	 * @return The current radius, or 0 if not in single mode
	 */
	int radius() const;
};

inline int AABBBrush::radius() const {
	if (!anySingleMode()) {
		return 0;
	}
	return _radius;
}

inline bool AABBBrush::centerMode() const {
	return isMode(BRUSH_MODE_CENTER);
}

inline void AABBBrush::setCenterMode() {
	setMode(BRUSH_MODE_CENTER);
}

inline bool AABBBrush::anySingleMode() const {
	return singleMode() || singleModeMove();
}

inline bool AABBBrush::singleMode() const {
	return isMode(BRUSH_MODE_SINGLE);
}

inline void AABBBrush::setSingleMode() {
	setMode(BRUSH_MODE_SINGLE);
}

inline bool AABBBrush::singleModeMove() const {
	return isMode(BRUSH_MODE_SINGLE_MOVE);
}

inline void AABBBrush::setSingleModeMove() {
	setMode(BRUSH_MODE_SINGLE_MOVE);
}

inline bool AABBBrush::aabbMode() const {
	return isMode(BRUSH_MODE_AABB);
}

inline void AABBBrush::setAABBMode() {
	setMode(BRUSH_MODE_AABB);
}

} // namespace voxedit
