/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include <glm/vec3.hpp>

namespace voxedit {

/**
 * @brief Build a thin closed polyline at brush radius, draped onto exposed surface voxels.
 *
 * Samples a circle in the cursor-face UV plane and snaps each sample to the nearest
 * exposed surface along the face axis so the ring follows terrain / slopes.
 * Used by any brush that reports a preview radius.
 */
void buildSurfaceRadiusOutline(const voxel::RawVolume *volume, const glm::ivec3 &center, voxel::FaceNames face,
							   int radius, core::Buffer<glm::vec3> &out);

} // namespace voxedit
