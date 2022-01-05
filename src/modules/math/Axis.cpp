/**
 * @file
 */

#include "Axis.h"
#include "core/Log.h"
#include "core/StringUtil.h"

namespace math {

math::Axis toAxis(const core::String &axisStr) {
	const char axisChr = core::string::toLower(axisStr[0]);
	math::Axis axis = math::Axis::None;
	switch (axisChr) {
	case 'x':
		axis = math::Axis::X;
		break;
	case 'y':
		axis = math::Axis::Y;
		break;
	case 'z':
		axis = math::Axis::Z;
		break;
	default:
		break;
	}
	if (axis == math::Axis::None) {
		Log::warn("Invalid axis given (valid are x, y and z)");
	}
	return axis;
}

} // namespace math
