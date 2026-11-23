/**
 * @file
 */

#include "RGBA.h"
#include "core/Common.h"
#include "math/U8Vec4.h"

namespace color {

RGBA RGBA::mix(const color::RGBA rgba1, const color::RGBA rgba2, float t) {
	if (rgba1 == rgba2) {
		return rgba1;
	}
	const math::U8Vec4 c1(rgba1.r, rgba1.g, rgba1.b, rgba1.a);
	const math::U8Vec4 c2(rgba2.r, rgba2.g, rgba2.b, rgba2.a);
	const math::U8Vec4 mixed = math::mix(c1, c2, t);
	return color::RGBA(mixed.r, mixed.g, mixed.b, core_max(rgba1.a, rgba2.a));
}

} // namespace color
