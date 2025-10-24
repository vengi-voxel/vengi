/**
 * @file
 */

#pragma once

#include "math/CoordinateSystem.h"
#include "scenegraph/SceneGraph.h"

namespace scenegraph {

/**
 * @param[in] from This specifies the coordinate system of the format
 * @param[in] to This specifies the target coordinate system
 * @param[in,out] sceneGraph The scene graph to convert
 * @note This does not update the volume coordinates, only the node transforms.
 */
bool convertCoordinateSystem(math::CoordinateSystem from, math::CoordinateSystem to,
							 scenegraph::SceneGraph &sceneGraph);

/**
 * @param[in] from This specifies the coordinate system of the format and is used to perform the transform
 * into the coordinate system of vengi (x right, y up, z back) @c CoordinateSystem::Vengi
 * @param[in,out] sceneGraph The scene graph to convert
 * @note This does not update the volume coordinates, only the node transforms.
 */
inline bool convertCoordinateSystem(math::CoordinateSystem from, scenegraph::SceneGraph &sceneGraph) {
	return convertCoordinateSystem(from, math::CoordinateSystem::Vengi, sceneGraph);
}

/**
 * @param[in] from This specifies the coordinate system of the format and is used to perform the transform
 * into the coordinate system of vengi (x right, y up, z back) @c CoordinateSystem::Vengi
 * @param[in] fromTransform The matrix to convert to the target coordinate system
 * @note This does not update the volume coordinates, only the node transforms.
 */
SceneGraphTransform convertCoordinateSystem(math::CoordinateSystem from, const SceneGraphTransform &fromTransform);

} // namespace scenegraph
