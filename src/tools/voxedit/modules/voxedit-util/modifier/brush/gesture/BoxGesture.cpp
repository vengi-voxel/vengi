/**
 * @file
 */

#include "BoxGesture.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "math/Vec3i.h"

namespace voxedit {

math::Vec3i BoxGesture::snapToGrid(const math::Vec3i &position, const math::Vec3i &resolution) {
	math::Vec3i pos = position;
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
	const math::Vec3i &delta = region.getDimensionsInVoxels();
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
	_firstPosition = math::Vec3i(0);
	_secondPosition = math::Vec3i(0);
}

math::Vec3i BoxGesture::cursorPosition(const BrushContext &ctx, int radius) const {
	math::Vec3i pos = ctx.cursorPosition;
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
	const math::Vec3i pos = cursorPosition(ctx, 0);
	const math::Vec3i first = snapToGrid(_firstPosition, ctx.gridResolution);
	if (center) {
		const math::Vec3i delta = math::abs(pos - first);
		return voxel::Region(first - delta, first + delta);
	}
	const math::Vec3i mins = math::min(first, pos);
	const math::Vec3i maxs = math::max(first, pos) + (ctx.gridResolution - 1);
	return voxel::Region(mins, maxs);
}

} // namespace voxedit
