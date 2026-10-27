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
	if (_dirtyRegion.isValid()) {
		_pendingUndoRegion = _dirtyRegion;
	}
	_dirtyRegion = voxel::Region::InvalidRegion;
	_hasLastPosition = false;
	_active = false;
}

void StrokeGesture::reset() {
	_hasLastPosition = false;
	_active = false;
	_dirtyRegion = voxel::Region::InvalidRegion;
	_pendingUndoRegion = voxel::Region::InvalidRegion;
}

void StrokeGesture::accumulate(const voxel::Region &region) {
	if (!region.isValid()) {
		return;
	}
	if (_dirtyRegion.isValid()) {
		_dirtyRegion.accumulate(region);
	} else {
		_dirtyRegion = region;
	}
}

voxel::Region StrokeGesture::consumePendingUndoRegion() {
	const voxel::Region region = _pendingUndoRegion;
	_pendingUndoRegion = voxel::Region::InvalidRegion;
	return region;
}

voxel::Region StrokeGesture::region(const glm::ivec3 &position, int radius, const glm::ivec3 &gridResolution) {
	if (radius > 0) {
		return voxel::Region(position - radius, position + radius);
	}
	return voxel::Region(position, position + (gridResolution - 1));
}

} // namespace voxedit
