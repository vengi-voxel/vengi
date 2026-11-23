/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "voxelutil/VolumeVisitor.h"
#include "math/Vec3i.h"

namespace voxel {
class RawVolume;
} // namespace voxel

namespace voxelutil {

/**
 * @param createEmpty if @c true, for empty parts of the source volume empty volumes will be created, too. Otherwise
 * they will be ignored.
 */
[[nodiscard]] core::Buffer<voxel::RawVolume *> splitVolume(const voxel::RawVolume *volume,
																 const math::Vec3i &maxSize, bool createEmpty = false);

/**
 * @param order This defines the order in which the splitted objects are returned.
 */
[[nodiscard]] core::Buffer<voxel::RawVolume *> splitObjects(const voxel::RawVolume *v, VisitorOrder order = VisitorOrder::ZYX,
			 voxel::Connectivity connectivity = voxel::Connectivity::EighteenConnected);

} // namespace voxelutil
