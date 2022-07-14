/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include <functional>

namespace voxel {
class RawVolumeWrapper;
}

namespace voxelutil {

bool copyIntoRegion(const voxel::RawVolume &in, voxel::RawVolume &out, const voxel::Region &targetRegion);

bool copy(const voxel::RawVolume &in, const voxel::Region &inRegion, voxel::RawVolume &out,
		  const voxel::Region &outRegion);
/**
 * @brief Checks if there is a solid voxel around the given position
 */
bool isTouching(const voxel::RawVolume *volume, const glm::ivec3& pos);

/**
 * @brief Checks whether the given region of the volume is only filled with air
 * @return @c true if no blocking voxel is inside the region, @c false otherwise
 * @sa voxel::isBlocked()
 */
bool isEmpty(const voxel::RawVolume &in, const voxel::Region &region);

using FillPlaneCallback = std::function<voxel::Voxel(const glm::ivec3 &, const voxel::Region &, voxel::FaceNames)>;
void fillPlane(voxel::RawVolumeWrapper &in, const FillPlaneCallback &targetVoxel, const voxel::Voxel &searchedVoxel,
			   const glm::ivec3 &position, voxel::FaceNames face);

void fillPlane(voxel::RawVolumeWrapper &in, const image::ImagePtr &image, const voxel::Voxel &searchedVoxel,
			   const glm::ivec3 &position, voxel::FaceNames face);

void fillPlane(voxel::RawVolumeWrapper &in, const voxel::Voxel &targetVoxel, const voxel::Voxel &searchedVoxel,
			   const glm::ivec3 &position, voxel::FaceNames face);

void fillHollow(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel);

} // namespace voxelutil
