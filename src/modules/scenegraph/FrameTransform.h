/**
 * @file
 */

#pragma once

#include "math/Quatf.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"

namespace voxel {
class Region;
}

namespace scenegraph {

class FrameTransform {
private:
	mutable math::Vec3f _scale;
	mutable bool _scaleCalculated = false;
	math::Mat4f _matrix = math::Mat4f::identity();

	void resetCache() {
		_scaleCalculated = false;
	}

public:
	void setWorldMatrix(const math::Mat4f &m);
	/**
	 * @brief World matrix without pivot offset
	 * @sa calculateWorldMatrix()
	 */
	math::Mat4f worldMatrix() const;
	/**
	 * @brief World translation without pivot offset
	 */
	math::Vec3f worldTranslation() const;
	const math::Vec3f &worldScale() const;

	/**
	 * @brief Check if this is an identity transform
	 */
	bool isIdentity() const;

	/**
	 * @brief World matrix with pivot offset
	 * @sa worldMatrix()
	 */
	math::Mat4f calculateWorldMatrix(const math::Vec3f &normalizedPivot, const math::Vec3f &dimensions) const;

	void decompose(math::Vec3f &scale, math::Quatf &orientation, math::Vec3f &translation) const;
};

math::Vec3f calculateExtents(const math::Vec3f &dimensions);

} // namespace scenegraph
