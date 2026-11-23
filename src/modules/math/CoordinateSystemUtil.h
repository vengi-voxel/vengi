/**
 * @file
 */

#pragma once

#include "math/CoordinateSystem.h"
#include "math/Mat4f.h"
#include "math/Mat3f.h"

namespace math {

bool coordinateSystemToMatrix(CoordinateSystem sys, math::Mat4f &matrix);
bool coordinateSystemToRotationMatrix(CoordinateSystem sys, math::Mat3f &matrix);

/**
 * @param[in] from This specifies the coordinate system of the format
 * @param[in] to This specifies the target coordinate system
 * @param[in] fromMatrix The matrix to convert to the target coordinate system
 * @note This does not update the volume coordinates, only the node transforms.
 */
math::Mat4f convertCoordinateSystem(CoordinateSystem from, CoordinateSystem to, const math::Mat4f &fromMatrix);

/**
 * @param[in] from This specifies the coordinate system of the format and is used to perform the transform
 * into the coordinate system of vengi (x right, y up, z back) @c FormatCoordinateSystem::Vengi
 * @param[in] fromMatrix The matrix to convert to the target coordinate system
 * @note This does not update the volume coordinates, only the node transforms.
 */
inline math::Mat4f convertCoordinateSystem(CoordinateSystem from, const math::Mat4f &fromMatrix) {
	return convertCoordinateSystem(from, CoordinateSystem::Vengi, fromMatrix);
}

bool coordinateSystemTransformationMatrix(CoordinateSystem from, CoordinateSystem to, math::Mat4f &transformationMatrix1,
										  math::Mat4f &transformationMatrix2);

} // namespace math
