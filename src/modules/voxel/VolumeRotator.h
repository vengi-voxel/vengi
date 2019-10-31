/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include "math/Axis.h"

namespace voxel {

class RawVolume;
class Voxel;

/**
 * @brief Rotate the given volume by the given angles in degree
 */
extern RawVolume* rotateVolume(const RawVolume* source, const glm::vec3& angles, const Voxel& empty, const glm::vec3& pivot, bool increaseSize = true);
/**
 * @brief Rotate the given volume on the given axis by 90 degree. This method does not lose any voxels
 * @note The volume size might differ
 */
extern RawVolume* rotateAxis(const RawVolume* source, math::Axis axis);
/**
 * @brief Mirrors the given volume on the given axis
 */
extern RawVolume* mirrorAxis(const RawVolume* source, math::Axis axis);

}
