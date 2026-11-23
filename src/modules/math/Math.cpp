/**
 * @file
 */

#include "math/Polar.h"
#include "math/Mat4Ops.h"
#include "math/OBB.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace math {

void obbSetInverse(Mat4f &inv, const Mat4f &rotation) {
	inv = inverse(rotation);
}

math::Vec3f polarToVector(float longitude, float latitude) {
	float num = math::cos(latitude);
	float num2 = math::sin(latitude);
	float num3 = math::cos(longitude);
	float num4 = math::sin(longitude);
	return math::Vec3f(num3 * num, num2, num4 * num);
}

void vectorToPolar(const math::Vec3f& vector, float& longitude, float& latitude) {
	float num = math::sqrt(vector.x * vector.x + vector.z * vector.z);
	if (num > 0.0f) {
		longitude = math::atan(vector.z, vector.x);
		latitude = math::atan(vector.y, num);
	} else {
		longitude = 0.0f;
		latitude = math::atan(vector.y, num);
	}
}

math::Vec3i transform(const math::Mat4f &mat, const math::Vec3i &pos, const math::Vec3f &pivot) {
	const math::Vec4f v((float)pos.x - 0.5f - pivot.x, (float)pos.y - 0.5f - pivot.y, (float)pos.z - 0.5f - pivot.z,
					  1.0f);
	const math::Vec3f &e = math::Vec3f(mat * v) + 0.5f + pivot;
	const math::Vec3i f = math::floor(e);
	return f;
}

math::Vec3f transform(const math::Mat4f &mat, const math::Vec3f &pos, const math::Vec3f &pivot) {
	const math::Vec4f v(pos.x - pivot.x, pos.y - pivot.y, pos.z - pivot.z,
					  1.0f);
	return math::Vec3f(mat * v) + pivot;
}

} // namespace math
