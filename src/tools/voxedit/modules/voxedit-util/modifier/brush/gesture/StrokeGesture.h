/**
 * @file
 */

#pragma once

#include "voxel/Region.h"
#include "voxelutil/VolumeSelect.h"
#include "math/Vec3i.h"

namespace voxedit {

/**
 * @brief Samples a continuous voxel path independently of a brush.
 * Preview evaluation does not advance this input sampler. A stationary sample repeats the dab,
 * preserving time-based painting; this component does not impose an overlap policy.
 */
class StrokeGesture {
private:
	math::Vec3i _lastPosition{0};
	bool _hasLastPosition = false;
	bool _active = false;

public:
	void begin();
	void end();
	void reset();
	bool active() const {
		return _active;
	}
	bool sampled(const math::Vec3i &position) const {
		return _hasLastPosition && _lastPosition == position;
	}
	static voxel::Region region(const math::Vec3i &position, int radius, const math::Vec3i &gridResolution);

	template<class Visitor>
	void visit(const math::Vec3i &position, Visitor &&visitor) {
		if (_hasLastPosition && _lastPosition != position) {
			bool skipFirst = true;
			voxelutil::bresenham3d(_lastPosition, position, [&](const math::Vec3i &p) {
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
