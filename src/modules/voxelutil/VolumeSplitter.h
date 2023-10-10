/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/fwd.hpp>

namespace voxel {
class RawVolume;
} // namespace voxel

namespace voxelutil {

/**
 * @param createEmpty if @c true, for empty parts of the source volume empty volumes will be created, too. Otherwise
 * they will be ignored.
 */
void splitVolume(const voxel::RawVolume *volume, const glm::ivec3 &maxSize,
				 core::DynamicArray<voxel::RawVolume *> &rawVolumes, bool createEmpty = false);

/**
 * @param order This defines the order in which the splitted objects are returned.
 */
void splitObjects(const voxel::RawVolume *v, core::DynamicArray<voxel::RawVolume *> &rawVolumes,
				  VisitorOrder order = VisitorOrder::ZYX);

} // namespace voxelutil
