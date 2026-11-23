/**
 * @file
 */

#include "FrameTransform.h"
#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"
#include "math/QuatOps.h"
#include "math/Scalar.h"

namespace scenegraph {

void FrameTransform::setWorldMatrix(const math::Mat4f &m) {
	_matrix = m;
	resetCache();
}

math::Mat4f FrameTransform::worldMatrix() const {
	return _matrix;
}

bool FrameTransform::isIdentity() const {
	const math::Mat4f matrix = worldMatrix();
	return math::all(math::epsilonEqual(math::Vec3f(matrix[3]), math::Vec3f(0.0f), 0.001f)) &&
		   math::all(math::epsilonEqual(math::Vec3f(math::eulerAngles(math::quat_cast(matrix))), math::Vec3f(0.0f), 0.001f));
}

math::Mat4f FrameTransform::calculateWorldMatrix(const math::Vec3f &normalizedPivot, const math::Vec3f &dimensions) const {
	return math::translate(worldMatrix(), -normalizedPivot * dimensions);
}

const math::Vec3f &FrameTransform::worldScale() const {
	if (_scaleCalculated) {
		return _scale;
	}
	math::Vec3f sc;
	math::Quatf orientation;
	math::Vec3f translation;
	decompose(sc, orientation, translation);
	_scale = sc;
	_scaleCalculated = true;
	return _scale;
}

math::Vec3f FrameTransform::worldTranslation() const {
	return math::column(worldMatrix(), 3);
}

void FrameTransform::decompose(math::Vec3f &scale, math::Quatf &orientation, math::Vec3f &translation) const {
	math::Vec3f skew;
	math::Vec4f perspective;
	math::decompose(worldMatrix(), scale, orientation, translation, skew, perspective);
}

math::Vec3f calculateExtents(const math::Vec3f &dimensions) {
	return dimensions / 2.0f;
}

} // namespace scenegraph
