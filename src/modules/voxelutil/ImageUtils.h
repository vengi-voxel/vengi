/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/Face.h"

namespace voxel {
class RawVolumeWrapper;
class RawVolume;
class Voxel;
class Region;
}

namespace palette {
class PaletteLookup;
class Palette;
}

namespace voxelutil {

/**
 * @brief Import a heightmap with rgb being the surface color and alpha channel being the height
 */
void importColoredHeightmap(voxel::RawVolumeWrapper& volume, palette::PaletteLookup &palLookup, const image::ImagePtr& image, const voxel::Voxel &underground);
void importHeightmap(voxel::RawVolumeWrapper& volume, const image::ImagePtr& image, const voxel::Voxel &underground, const voxel::Voxel &surface);
/**
 * @param alphaAsHeight If this is @c true, the rgb color is used for the colors - otherwise the red channel is used
 * (because a gray scale image is expected)
 */
int importHeightMaxHeight(const image::ImagePtr &image, bool alphaAsHeight);
[[nodiscard]] voxel::RawVolume* importAsPlane(const image::ImagePtr& image, const palette::Palette &palette, uint8_t thickness = 1);
[[nodiscard]] voxel::RawVolume* importAsPlane(const image::ImagePtr& image, uint8_t thickness = 1);
[[nodiscard]] voxel::RawVolume* importAsPlane(const image::Image *image, const palette::Palette &palette, uint8_t thickness = 1);
[[nodiscard]] voxel::RawVolume* importAsPlane(const image::Image *image, uint8_t thickness = 1);
core::String getDefaultDepthMapFile(const core::String &imageName, const core::String &postfix = "-dm");
[[nodiscard]] voxel::RawVolume* importAsVolume(const image::ImagePtr& image, const palette::Palette &palette, uint8_t maxDepth, bool bothSides = false);
[[nodiscard]] voxel::RawVolume* importAsVolume(const image::ImagePtr& image, const image::ImagePtr& depthMap, const palette::Palette &palette, uint8_t maxDepth, bool bothSides = false);
[[nodiscard]] voxel::RawVolume* importAsVolume(const image::ImagePtr& image, uint8_t maxDepth, bool bothSides = false);
bool importFace(voxel::RawVolumeWrapper &volume, const voxel::Region &region, const palette::Palette &palette, voxel::FaceNames faceName, const image::ImagePtr &image, const glm::vec2 &uv0, const glm::vec2 &uv1, uint8_t replacementPalIdx = 0);

}
