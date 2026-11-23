/**
 * @file
 */

#pragma once

#include "math/Axis.h"
#include "voxel/VoxelSampling.h"
#include "math/Mat4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace voxel {
class RawVolume;
class Voxel;
} // namespace voxel

namespace palette {
class Palette;
}

namespace voxelutil {

[[nodiscard]] voxel::RawVolume *rotateVolume(const voxel::RawVolume *srcVolume, const math::Mat4f &mat,
											 const math::Vec3f &normalizedPivot, voxel::VoxelSampling sampling = voxel::VoxelSampling::Linear);

/**
 * @brief Rotate the given volume by the given angles in degree
 */
[[nodiscard]] voxel::RawVolume *rotateVolumeDegrees(const voxel::RawVolume *source, const math::Vec3i &angles,
											 const math::Vec3f &normalizedPivot, voxel::VoxelSampling sampling = voxel::VoxelSampling::Linear);
/**
 * @brief Rotate the given volume on the given axis by 90 degree. This method does not lose any voxels
 * @note The volume size might differ
 */
[[nodiscard]] voxel::RawVolume *rotateAxis(const voxel::RawVolume *source, math::Axis axis);
/**
 * @brief Mirrors the given volume on the given axis
 */
[[nodiscard]] voxel::RawVolume *mirrorAxis(const voxel::RawVolume *source, math::Axis axis);

} // namespace voxelutil
