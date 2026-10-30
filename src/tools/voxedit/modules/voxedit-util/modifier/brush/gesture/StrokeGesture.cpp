/**
 * @file
 */

#include "StrokeGesture.h"

namespace voxedit {

void StrokeGesture::begin() {
	reset();
	_active = true;
}

void StrokeGesture::end() {
	reset();
}

void StrokeGesture::reset() {
	_hasLastPosition = false;
	_active = false;
}

voxel::Region StrokeGesture::region(const glm::ivec3 &position, int radius, const glm::ivec3 &gridResolution) {
	if (radius > 0) {
		return voxel::Region(position - radius, position + radius);
	}
	return voxel::Region(position, position + (gridResolution - 1));
}

} // namespace voxedit
