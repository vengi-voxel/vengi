/**
 * @file
 */

#pragma once

#include "glm/geometric.hpp"
#include "image/Image.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include <glm/fwd.hpp>

namespace voxel {
class RawVolumeWrapper;
}
namespace palette {
class Palette;
}

namespace voxelutil {

/**
 * @brief Checks if there is a solid voxel around the given position
 */
bool isTouching(const voxel::RawVolume &volume, const glm::ivec3 &pos,
				voxel::Connectivity connectivity = voxel::Connectivity::SixConnected);

/**
 * @brief Checks whether the given region of the volume is only filled with air
 * @return @c true if no blocking voxel is inside the region, @c false otherwise
 * @sa voxel::isBlocked()
 */
bool isEmpty(const voxel::RawVolume &in, const voxel::Region &region);

/**
 * @brief Fill the plane at the position with the pixels of the image if the underlying voxel is of the given type
 */
int fillPlane(voxel::RawVolumeWrapper &in, const image::ImagePtr &image, const voxel::Voxel &searchedVoxel,
			  const glm::ivec3 &position, voxel::FaceNames face);

/**
 * @brief Extrudes a plane in a voxel volume.
 *
 * @param pos The position in the volume where the first voxel should be placed and where to start the extrusion from.
 * @param face The face where the trace enters the ground voxel. This is about the direction of the plane that is
 * extruded. If e.g. face x was detected, the plane is created on face y and z.
 * @param groundVoxel The voxel we want to extrude on
 * @param newPlaneVoxel The voxel to put at the given position
 */
int extrudePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
				 const voxel::Voxel &groundVoxel, const voxel::Voxel &newPlaneVoxel, int thickness);
/**
 * @return The region of the volume that was changed
 */
voxel::Region extrudePlaneRegion(const voxel::RawVolume &volume, const glm::ivec3 &pos, voxel::FaceNames face,
								 const voxel::Voxel &groundVoxel, const voxel::Voxel &newPlaneVoxel, int thickness);

/**
 * @brief Erases a plane in a voxel volume.
 *
 * This function identifies a plane in the voxel volume based on the given position and face direction.
 * It then erases the voxels in this plane that match a specified voxel, replacing them with empty voxels.
 *
 * @param in The voxel volume to erase from.
 * @param pos The position in the voxel volume to start the erasing from.
 * @param face The direction of the face to erase.
 * @param groundVoxel The voxel to match for erasing.
 * @return The number of voxels that were erased.
 */
int erasePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &groundVoxel, int thickness = 1);
/**
 * @return The region of the volume that was changed
 */
voxel::Region erasePlaneRegion(const voxel::RawVolume &volume, const glm::ivec3 &pos, voxel::FaceNames face,
							   const voxel::Voxel &groundVoxel, int thickness = 1);

/**
 * @brief Paints a plane of existing voxels in a voxel volume with a specified voxel.
 *
 * This function identifies a plane in the voxel volume based on the given position and face direction.
 * It then paints the voxels in this plane with a specified voxel.
 *
 * @param in The voxel volume to paint.
 * @param pos The position in the voxel volume to start the painting from.
 * @param face The direction of the face to paint.
 * @param replaceVoxel The voxel to paint the plane with.
 */
int paintPlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
			   const voxel::Voxel &searchVoxel, const voxel::Voxel &replaceVoxel);

/**
 * @brief Overrides existing voxels on a plane in a voxel volume with a specified voxel.
 *
 * This function identifies a plane in the voxel volume based on the given position and face direction.
 * It then overrides the voxels in this plane with a specified voxel.
 *
 * @param in The voxel volume to override.
 * @param pos The position in the voxel volume to start the override from.
 * @param face The direction of the face to override.
 * @param replaceVoxel The voxel to override the plane with.
 */
int overridePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face,
				  const voxel::Voxel &replaceVoxel, int thickness = 1);
/**
 * @return The region of the volume that was changed
 */
voxel::Region overridePlaneRegion(const voxel::RawVolume &volume, const glm::ivec3 &pos, voxel::FaceNames face,
								  const voxel::Voxel &replaceVoxel, int thickness = 1);

void fill(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel, bool overwrite = true);
void clear(voxel::RawVolumeWrapper &in);

/**
 * @brief Remaps or converts the voxel colors to the new given palette by searching for the closest color
 * @param skipColorIndex One particular palette color index that is not taken into account. This can be used to e.g.
 * search for replacements
 * @return The region of the volume that was changed
 */
voxel::Region remapToPalette(voxel::RawVolume *v, const palette::Palette &oldPalette,
							 const palette::Palette &newPalette, int skipColorIndex = -1);

/**
 * @brief Creates a diff between the two given volumes
 * @note The caller has to free the volume
 * @return nullptr if the volumes don't differ in the shared region dimensions
 */
[[nodiscard]] voxel::RawVolume *diffVolumes(const voxel::RawVolume *v1, const voxel::RawVolume *v2);

[[nodiscard]] voxel::RawVolume *applyTransformToVolume(const voxel::RawVolume &volume, const glm::mat4 &transform,
														 const glm::vec3 &pivot);

} // namespace voxelutil
