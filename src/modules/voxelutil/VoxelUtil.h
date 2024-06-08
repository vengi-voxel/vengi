/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/Face.h"
#include "voxel/Voxel.h"
#include "voxelutil/Connectivity.h"
#include <glm/fwd.hpp>

namespace voxel {
class RawVolume;
class Region;
class Voxel;
class RawVolumeWrapper;
}
namespace palette {
class Palette;
}

namespace voxelutil {

/**
 * @brief Copies the entire input voxel volume into a specified region in the output voxel volume.
 *
 * This function is a convenience wrapper around the 'copy' function. It copies the entire input volume into a specified region in the output volume.
 *
 * @param in The input voxel volume.
 * @param out The output voxel volume.
 * @param targetRegion The region to copy to in the output volume.
 * @return true if the copied region in the output volume is valid, false otherwise.
 */
bool copyIntoRegion(const voxel::RawVolume &in, voxel::RawVolume &out, const voxel::Region &targetRegion);

/**
 * @brief The interpolated voxel for the given position
 *
 * @param v The voxel volume to fill.
 * @param pos The position in the voxel volume to get the interpolated voxel for.
 * @param palette The palette used for interpolation.
 */
voxel::Voxel getInterpolated(voxel::RawVolumeWrapper &v, const glm::ivec3 &pos, const palette::Palette &palette);

/**
 * @brief Copies a region from one voxel volume to another.
 *
 * This function copies a specified region from the input voxel volume to a specified region in the output voxel volume.
 * The regions are defined by their lower and upper corners. The function also marks the region in the output volume as dirty.
 *
 * @param in The input voxel volume.
 * @param inRegion The region to copy from the input volume.
 * @param out The output voxel volume.
 * @param outRegion The region to copy to in the output volume.
 * @return true if the copied region in the output volume is valid, false otherwise.
 */
bool copy(const voxel::RawVolume &in, const voxel::Region &inRegion, voxel::RawVolume &out,
		  const voxel::Region &outRegion);

/**
 * @brief Checks if there is a solid voxel around the given position
 */
bool isTouching(const voxel::RawVolume &volume, const glm::ivec3& pos, Connectivity connectivity = Connectivity::SixConnected);

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
int extrudePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face, const voxel::Voxel &groundVoxel,
			const voxel::Voxel &newPlaneVoxel, int thickness);

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
int erasePlane(voxel::RawVolumeWrapper &in, const glm::ivec3 &pos, voxel::FaceNames face, const voxel::Voxel &groundVoxel);

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
				  const voxel::Voxel &replaceVoxel);

/**
 * @brief Fills the hollow spaces in a voxel volume.
 *
 * This function iterates over the voxel volume and identifies hollows that are totally enclosed by existing voxels.
 * It then fills these hollow spaces with a specified voxel.
 *
 * @param[in,out] in The voxel volume to fill.
 * @param[in] voxel The voxel to fill the hollow spaces with.
 */
void fillHollow(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel);
void hollow(voxel::RawVolumeWrapper &in);
void fill(voxel::RawVolumeWrapper &in, const voxel::Voxel &voxel, bool overwrite = true);
void clear(voxel::RawVolumeWrapper &in);

/**
 * @brief Remaps or converts the voxel colors to the new given palette by searching for the closest color
 * @param skipColorIndex One particular palette color index that is not taken into account. This can be used to e.g. search for replacements
 */
voxel::Region remapToPalette(voxel::RawVolume *v, const palette::Palette &oldPalette, const palette::Palette &newPalette, int skipColorIndex = -1);

/**
 * @brief Creates a diff between the two given volumes
 * @note The caller has to free the volume
 * @return nullptr if the volumes don't differ in the shared region dimensions
 */
voxel::RawVolume *diffVolumes(const voxel::RawVolume *v1, const voxel::RawVolume *v2);

} // namespace voxelutil
