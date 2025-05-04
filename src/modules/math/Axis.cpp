/**
 * @file
 */

#include "Axis.h"
#include "core/Log.h"

namespace math {

math::Axis toAxis(const core::String &axesStr) {
	const core::String axes = axesStr.toLower();
	math::Axis axesMask = math::Axis::None;
	for (size_t i = 0; i < axes.size(); ++i) {
		const char axisChr = axes[i];
		switch (axisChr) {
		case 'x':
			axesMask |= math::Axis::X;
			break;
		case 'y':
			axesMask |= math::Axis::Y;
			break;
		case 'z':
			axesMask |= math::Axis::Z;
			break;
		default:
			break;
		}
	}
	if (axesMask == math::Axis::None) {
		Log::warn("Invalid axis given (valid are x, y and z)");
	}
	return axesMask;
}

} // namespace math
