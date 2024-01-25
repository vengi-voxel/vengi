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
	BRUSH_MODE_SINGLE = 2
};
// extend your own flags by using this macro for the first shift value
#define BRUSH_MODE_CUSTOM 3

/**
 * @brief A brush that operates on an axis aligned bounding box
 */
class AABBBrush : public Brush {
private:
	using Super = Brush;

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
	math::Axis _mirrorAxis = math::Axis::None;
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
	 * The second position of the aabb
	 */
	glm::ivec3 _aabbSecondPos{0};

	/**
	 * The mirror position is based on the reference position whenever the mirror axis is set
	 */
	glm::ivec3 _mirrorPos{0};

	math::Axis getShapeDimensionForAxis(voxel::FaceNames face, const glm::ivec3 &dimensions, int &width, int &height,
										int &depth) const;
	virtual bool generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						  const BrushContext &context, const voxel::Region &region) = 0;
	glm::ivec3 applyGridResolution(const glm::ivec3 &inPos, int resolution) const;
	void toggleMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos);

	bool isMode(uint32_t flag) const;
	void setMode(uint32_t flag);

public:
	AABBBrush(BrushType type, ModifierType defaultModifier = ModifierType::Place,
			  ModifierType supportedModifiers = (ModifierType::Place | ModifierType::Erase | ModifierType::Override));
	virtual ~AABBBrush() = default;
	void construct() override;
	void reset() override;
	void update(const BrushContext &ctx, double nowSeconds) override;

	bool getMirrorAABB(glm::ivec3 &mins, glm::ivec3 &maxs) const;
	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
				 const BrushContext &context) override;
	/**
	 * @return the current position in a multi action execution
	 * @sa needsFurtherAction()
	 * @sa executeAdditionalAction()
	 */
	glm::ivec3 currentCursorPosition(const glm::ivec3 &cursorPosition) const;

	voxel::Region calcRegion(const BrushContext &context) const;

	/**
	 * @brief Allows to override the default behaviour to span an AABB while holding the mouse button.
	 * @note This allows us to disable the AABB behaviour in some cases, e.g. when single mode is activated
	 */
	virtual bool wantAABB() const;
	/**
	 * @brief Will set the first position of the aabb
	 * @note This is used in input methods or @c ActionButton implementations
	 * @sa step()
	 * @sa stop()
	 */
	bool start(const BrushContext &context);
	/**
	 * @brief Will set the second position of the aabb (only after @c start() was called, and not if @c setRegion() was
	 * used).
	 * @note This is used in input methods or @c ActionButton implementations
	 * @sa start()
	 * @sa stop()
	 */
	void step(const BrushContext &context);
	void stop(const BrushContext &context);
	/**
	 * @return @c true if @c start() was called without calling @c stop() or aborting the action otherwise
	 */
	bool active() const override;
	bool aborted(const BrushContext &context) const;
	/**
	 * @return @c true if the aabb has the size of 1 in one direction. This means that the second position can still be
	 * modified.
	 */
	bool needsFurtherAction(const BrushContext &context) const;

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

	void setAABBMode();
	bool aabbMode() const;

	bool setMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos);
	math::Axis mirrorAxis() const;
	const glm::ivec3 &mirrorPos() const;

	int radius() const;
	void setRadius(int radius);
};

inline int AABBBrush::radius() const {
	if (!isMode(BRUSH_MODE_SINGLE)) {
		return 0;
	}
	return _radius;
}

inline const glm::ivec3 &AABBBrush::mirrorPos() const {
	return _mirrorPos;
}

inline math::Axis AABBBrush::mirrorAxis() const {
	return _mirrorAxis;
}

inline bool AABBBrush::centerMode() const {
	return isMode(BRUSH_MODE_CENTER);
}

inline void AABBBrush::setCenterMode() {
	setMode(BRUSH_MODE_CENTER);
}

inline bool AABBBrush::singleMode() const {
	return isMode(BRUSH_MODE_SINGLE);
}

inline void AABBBrush::setSingleMode() {
	setMode(BRUSH_MODE_SINGLE);
}

inline bool AABBBrush::aabbMode() const {
	return isMode(BRUSH_MODE_AABB);
}

inline void AABBBrush::setAABBMode() {
	setMode(BRUSH_MODE_AABB);
}

} // namespace voxedit
