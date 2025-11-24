/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "voxel/Face.h"
#include "voxel/Region.h"

namespace voxedit {

enum BrushFlags : uint32_t {
	BRUSH_MODE_AABB = 0,
	/**
	 * span the aabb around the first position - so not only maxs depend on the second position, but
	 * also the mins.
	 */
	BRUSH_MODE_CENTER = 1,
	// continously set voxels until the action button is released
	BRUSH_MODE_SINGLE = 2,
	BRUSH_MODE_SINGLE_MOVE = 3,

	// extend your own flags by using this macro for the first shift value
	BRUSH_MODE_CUSTOM = 4,
};

/**
 * @brief A brush that operates on an axis aligned bounding box
 * @ingroup Brushes
 */
class AABBBrush : public Brush {
private:
	using Super = Brush;

	glm::ivec3 _lastCursorPos{-100000};

protected:
	/**
	 * @c true if the current action spans an aabb. This first position of the aabb is set now.
	 */
	bool _aabbMode = false;
	/**
	 * If this is true, the aabb has a valid mins and maxs already, but the maxs
	 * can still be changed as long as @c step() is called.
	 */
	bool _secondPosValid = false;
	/**
	 * if the current modifier type allows or needs a second action to span the
	 * volume to operate in, this is the direction into which the second action
	 * points
	 */
	voxel::FaceNames _aabbFace = voxel::FaceNames::Max;

	uint32_t _mode = BRUSH_MODE_AABB;
	int _radius = 0;

	/**
	 * The first position of the aabb
	 */
	glm::ivec3 _aabbFirstPos{0};
	/**
	 * The second position of the aabb - the cursor position can now be used to span the 3rd dimension of the aabb.
	 * The first and second position only define 2 dimensions.
	 */
	glm::ivec3 _aabbSecondPos{0};

	glm::ivec3 applyGridResolution(const glm::ivec3 &inPos, int resolution) const;
	voxel::Region extendRegionInOrthoMode(const voxel::Region &brushRegion, const voxel::Region &volumeRegion,
										  const BrushContext &ctx) const;

	bool isMode(uint32_t flag) const;
	void setMode(uint32_t flag);

	/**
	 * @brief Allows to override the default behaviour to span an AABB while holding the mouse button.
	 * @note This allows us to disable the AABB behaviour in some cases, e.g. when single mode is activated
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
	 * @return the current position in a multi action execution
	 * @sa needsAdditionalAction()
	 * @sa executeAdditionalAction()
	 */
	glm::ivec3 currentCursorPosition(const BrushContext &ctx) const;

	voxel::Region calcRegion(const BrushContext &ctx) const override;
	/**
	 * @brief Will set the first position of the aabb
	 * @note This is used in input methods or @c ActionButton implementations
	 * @sa step()
	 * @sa stop()
	 */
	virtual bool start(const BrushContext &ctx);
	/**
	 * @brief Will set the second position of the aabb (only after @c start() was called).
	 * @note This is used in input methods or @c ActionButton implementations
	 * @sa start()
	 * @sa stop()
	 */
	void step(const BrushContext &ctx);
	void stop(const BrushContext &ctx);
	/**
	 * @return @c true if @c start() was called without calling @c stop() or aborting the action otherwise
	 */
	bool active() const override;
	bool aborted(const BrushContext &ctx) const;
	/**
	 * @return @c true if the aabb has the size of 1 in one direction. This means that the last
	 * position can still be modified to span a full volume on all three axes.
	 */
	virtual bool needsAdditionalAction(const BrushContext &ctx) const;

	/**
	 * @brief The modifier can build the aabb from the center of the current
	 * cursor position.
	 * Set this to @c true to activate this. The default is to build the aabb
	 * from the corner(s)
	 */
	void setCenterMode();
	bool centerMode() const;

	void setSingleMode();
	bool singleMode() const;

	void setSingleModeMove();
	bool singleModeMove() const;

	bool anySingleMode() const;

	void setAABBMode();
	bool aabbMode() const;

	int radius() const;
	void setRadius(int radius);
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
