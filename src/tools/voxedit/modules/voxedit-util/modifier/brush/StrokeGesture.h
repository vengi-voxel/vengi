/**
 * @file
 */

#pragma once

#include "voxel/Region.h"
#include "voxelutil/VolumeSelect.h"

namespace voxedit {

/**
 * @brief Samples a continuous voxel path and accumulates its undo region independently of a brush.
 * Preview evaluation must not call visit() or accumulate(). A stationary sample repeats the dab,
 * preserving time-based painting; this component does not impose an overlap policy.
 */
class StrokeGesture {
private:
	glm::ivec3 _lastPosition{0};
	bool _hasLastPosition = false;
	bool _active = false;
	voxel::Region _dirtyRegion = voxel::Region::InvalidRegion;
	voxel::Region _pendingUndoRegion = voxel::Region::InvalidRegion;

public:
	void begin();
	void end();
	void reset();
	bool active() const {
		return _active;
	}
	void accumulate(const voxel::Region &region);
	voxel::Region consumePendingUndoRegion();
	static voxel::Region region(const glm::ivec3 &position, int radius, const glm::ivec3 &gridResolution);

	template<class Visitor>
	void visit(const glm::ivec3 &position, Visitor &&visitor) {
		if (_hasLastPosition && _lastPosition != position) {
			bool skipFirst = true;
			voxelutil::bresenham3d(_lastPosition, position, [&](const glm::ivec3 &p) {
				if (skipFirst) {
					skipFirst = false;
					return;
				}
				visitor(p);
			});
		} else {
			visitor(position);
		}
		_lastPosition = position;
		_hasLastPosition = true;
	}
};

} // namespace voxedit
