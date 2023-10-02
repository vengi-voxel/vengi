/**
 * @file
 */

#pragma once

#include "../ShapeType.h"
#include "Brush.h"
#include "voxel/Face.h"

namespace voxedit {

class ShapeBrush : public Brush {
private:
	using Super = Brush;

protected:
	ShapeType _shapeType = ShapeType::AABB;
	/**
	 * @c true if the current action spans an aabb. This first position of the aabb is set now.
	 */
	bool _aabbMode = false;
	/**
	 * @c true means to span the aabb around the first position - so not only maxs depend on the second position, but
	 * also the mins.
	 */
	bool _center = false;
	bool _single = false;
	/**
	 * The aabb is not created by @c start() and @c step(), but set from the outside
	 * @sa setRegion()
	 */
	bool _fixedRegion = false;
	/**
	 * If this is true, the aabb has a valid mins and maxs already, but the maxs
	 * can still be changed as long as @c step() is called.
	 */
	bool _secondPosValid = false;

	/**
	 * The first position of the aabb
	 */
	glm::ivec3 _aabbFirstPos{0};
	/**
	 * The second position of the aabb
	 */
	glm::ivec3 _aabbSecondPos{0};

	math::Axis _mirrorAxis = math::Axis::None;
	/**
	 * The mirror position is based on the reference position whenever the mirror axis is set
	 */
	glm::ivec3 _mirrorPos{0};

	/**
	 * if the current modifier type allows or needs a second action to span the
	 * volume to operate in, this is the direction into which the second action
	 * points
	 */
	voxel::FaceNames _aabbFace = voxel::FaceNames::Max;

	math::Axis getShapeDimensionForAxis(voxel::FaceNames face, const glm::ivec3 &dimensions, int &width, int &height,
										int &depth) const;
	bool generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const voxel::Region &region,
				  const voxel::Voxel &voxel);
	glm::ivec3 applyGridResolution(const glm::ivec3 &inPos, int resolution) const;
	void setShapeType(ShapeType type);
	void toggleMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos);

public:
	void construct() override;
	void reset() override;

	ShapeType shapeType() const;
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
	void setCenterMode(bool center);
	bool centerMode() const;

	void setSingleMode(bool single);
	bool singleMode() const;

	bool setMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos);
	math::Axis mirrorAxis() const;
	const glm::ivec3 &mirrorPos() const;

	/**
	 * @brief Can be used to set a fixed sized region. In this case the @c step() method won't modify the first and
	 * second position of the aabb anymore. Can be used to set the region via script or ui.
	 */
	void setRegion(const voxel::Region &region, voxel::FaceNames face);
};

inline const glm::ivec3 &ShapeBrush::mirrorPos() const {
	return _mirrorPos;
}

inline math::Axis ShapeBrush::mirrorAxis() const {
	return _mirrorAxis;
}

inline bool ShapeBrush::centerMode() const {
	return _center;
}

inline void ShapeBrush::setCenterMode(bool center) {
	_center = center;
}

inline bool ShapeBrush::singleMode() const {
	return _single;
}

inline void ShapeBrush::setSingleMode(bool single) {
	_single = single;
}

} // namespace voxedit
