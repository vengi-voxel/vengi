/**
 * @file
 */

#pragma once

#include "voxel/Face.h"
#include "voxel/Region.h"

namespace voxedit {

struct BrushContext;

/**
 * @brief Tracks the anchors and face of a multi-step box gesture independently of the brush algorithm.
 * After capturing the second corner, cursor movement only changes the depth along the initial face.
 */
class BoxGesture {
private:
	bool _active = false;
	bool _secondPositionValid = false;
	voxel::FaceNames _face = voxel::FaceNames::Max;
	glm::ivec3 _firstPosition{0};
	glm::ivec3 _secondPosition{0};

public:
	static glm::ivec3 snapToGrid(const glm::ivec3 &position, const glm::ivec3 &resolution);
	static bool needsDepth(const BrushContext &ctx, const voxel::Region &region);

	void begin(const BrushContext &ctx, bool active);
	/** End the gesture while retaining its anchors for callers that evaluate after release. */
	void end();
	void reset();
	/** Stop spanning without changing the captured face or anchors. */
	void stopSpanning();
	bool step(const BrushContext &ctx, int radius);
	glm::ivec3 cursorPosition(const BrushContext &ctx, int radius) const;
	voxel::Region region(const BrushContext &ctx, bool center) const;

	bool active() const {
		return _active;
	}
	bool hasSecondPosition() const {
		return _secondPositionValid;
	}
	void clearSecondPosition() {
		_secondPositionValid = false;
	}
	voxel::FaceNames face() const {
		return _face;
	}
	void setFace(voxel::FaceNames face) {
		_face = face;
	}
	const glm::ivec3 &firstPosition() const {
		return _firstPosition;
	}
	/** Plane extrusion supplies the surface bounds instead of capturing them with the mouse. */
	void setPlaneBounds(const voxel::Region &region);
};

} // namespace voxedit
