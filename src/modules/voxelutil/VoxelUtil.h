/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
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

using WalkCheckCallback = std::function<bool(const voxel::RawVolumeWrapper &, const glm::ivec3 &)>;
using WalkExecCallback = std::function<bool(voxel::RawVolumeWrapper &, const glm::ivec3 &)>;

int fillPlane(voxel::RawVolumeWrapper &in, const image::ImagePtr &image, const voxel::Voxel &searchedVoxel,
			  const glm::ivec3 &position, voxel::FaceNames face);

/**
 * @param pos The position where the first voxel should be placed
 * @param face The face where the trace enters the ground voxel. This is about the direction of the plane that is
 * extruded. If e.g. face x was detected, the plane is created on face y and z.
 * @param groundVoxel The voxel we want to extrude on
 * @param newPlaneVoxel The voxel to put at the given position
 */
int extrudePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face, const voxel::Voxel &groundVoxel,
			const voxel::Voxel &newPlaneVoxel);
int erasePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face, const voxel::Voxel &groundVoxel);
int paintPlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &searchVoxel, const voxel::Voxel &replaceVoxel);

void fillHollow(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel);

} // namespace voxelutil
