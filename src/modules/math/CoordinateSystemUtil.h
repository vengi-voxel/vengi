/**
 * @file
 */

#pragma once

#include "math/CoordinateSystem.h"
#include <glm/mat4x4.hpp>

namespace math {

bool coordinateSystemToMatrix(CoordinateSystem sys, glm::mat4 &matrix);

/**
 * @param[in] from This specifies the coordinate system of the format
 * @param[in] to This specifies the target coordinate system
 * @param[in] fromMatrix The matrix to convert to the target coordinate system
 * @note This does not update the volume coordinates, only the node transforms.
 */
glm::mat4 convertCoordinateSystem(CoordinateSystem from, CoordinateSystem to, const glm::mat4 &fromMatrix);

/**
 * @param[in] from This specifies the coordinate system of the format and is used to perform the transform
 * into the coordinate system of vengi (x right, y up, z back) @c FormatCoordinateSystem::Vengi
 * @param[in] fromMatrix The matrix to convert to the target coordinate system
 * @note This does not update the volume coordinates, only the node transforms.
 */
inline glm::mat4 convertCoordinateSystem(CoordinateSystem from, const glm::mat4 &fromMatrix) {
	return convertCoordinateSystem(from, CoordinateSystem::Vengi, fromMatrix);
}

bool coordinateSystemTransformationMatrix(CoordinateSystem from, CoordinateSystem to, glm::mat4 &transformationMatrix1,
										  glm::mat4 &transformationMatrix2);

} // namespace math
