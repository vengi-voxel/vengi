/**
 * @file
 */

#include "BoxGesture.h"
#include "Brush.h"

namespace voxedit {

glm::ivec3 BoxGesture::snapToGrid(const glm::ivec3 &position, const glm::ivec3 &resolution) {
	glm::ivec3 pos = position;
	for (int i = 0; i < 3; ++i) {
		if (resolution[i] > 0 && pos[i] % resolution[i] != 0) {
			pos[i] = (pos[i] / resolution[i]) * resolution[i];
		}
	}
	return pos;
}

bool BoxGesture::needsDepth(const BrushContext &ctx, const voxel::Region &region) {
	if (ctx.lockedAxis != math::Axis::None) {
		return false;
	}
	const glm::ivec3 &delta = region.getDimensionsInVoxels();
	int greater = 0;
	int equal = 0;
	for (int i = 0; i < 3; ++i) {
		if (delta[i] > ctx.gridResolution[i]) {
			++greater;
		} else if (delta[i] == ctx.gridResolution[i]) {
			++equal;
		}
	}
	return greater == 2 && equal == 1;
}

void BoxGesture::begin(const BrushContext &ctx, bool active) {
	_firstPosition = snapToGrid(ctx.cursorPosition, ctx.gridResolution);
	_secondPositionValid = false;
	_active = active;
	_face = ctx.cursorFace;
}

void BoxGesture::stopSpanning() {
	_active = false;
	_secondPositionValid = false;
}

void BoxGesture::end() {
	stopSpanning();
	_face = voxel::FaceNames::Max;
}

void BoxGesture::reset() {
	end();
	_firstPosition = glm::ivec3(0);
	_secondPosition = glm::ivec3(0);
}

glm::ivec3 BoxGesture::cursorPosition(const BrushContext &ctx, int radius) const {
	glm::ivec3 pos = ctx.cursorPosition;
	if (_secondPositionValid) {
		if (radius > 0) {
			return _secondPosition;
		}
		const math::Axis axis = voxel::faceToAxis(_face);
		if (axis != math::Axis::None) {
			const int idx = math::getIndexForAxis(axis);
			pos[(idx + 1) % 3] = _secondPosition[(idx + 1) % 3];
			pos[(idx + 2) % 3] = _secondPosition[(idx + 2) % 3];
		}
	}
	return pos;
}

bool BoxGesture::step(const BrushContext &ctx, int radius) {
	if (!_active || radius > 0 || ctx.lockedAxis != math::Axis::None) {
		return false;
	}
	_secondPosition = cursorPosition(ctx, radius);
	const bool captured = !_secondPositionValid;
	_secondPositionValid = true;
	return captured;
}

void BoxGesture::setPlaneBounds(const voxel::Region &region) {
	_firstPosition = region.getLowerCorner();
	_secondPosition = region.getUpperCorner();
	_secondPositionValid = true;
}

voxel::Region BoxGesture::region(const BrushContext &ctx, bool center) const {
	const glm::ivec3 pos = cursorPosition(ctx, 0);
	const glm::ivec3 first = snapToGrid(_firstPosition, ctx.gridResolution);
	if (center) {
		const glm::ivec3 delta = glm::abs(pos - first);
		return voxel::Region(first - delta, first + delta);
	}
	const glm::ivec3 mins = glm::min(first, pos);
	const glm::ivec3 maxs = glm::max(first, pos) + (ctx.gridResolution - 1);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
