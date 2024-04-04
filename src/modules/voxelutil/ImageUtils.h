/**
 * @file
 */

#pragma once

#include "image/Image.h"

namespace voxel {
class RawVolumeWrapper;
class RawVolume;
class Voxel;
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
int importHeightMaxHeight(const image::ImagePtr &image, bool alpha);
voxel::RawVolume* importAsPlane(const image::ImagePtr& image, const palette::Palette &palette, uint8_t thickness = 1);
voxel::RawVolume* importAsPlane(const image::ImagePtr& image, uint8_t thickness = 1);
voxel::RawVolume* importAsPlane(const image::Image *image, const palette::Palette &palette, uint8_t thickness = 1);
voxel::RawVolume* importAsPlane(const image::Image *image, uint8_t thickness = 1);
voxel::RawVolume* importAsVolume(const image::ImagePtr& image, const palette::Palette &palette, uint8_t maxDepth, bool bothSides = false);
voxel::RawVolume* importAsVolume(const image::ImagePtr& image, const image::ImagePtr& heightmap, const palette::Palette &palette, uint8_t maxDepth, bool bothSides = false);
voxel::RawVolume* importAsVolume(const image::ImagePtr& image, uint8_t maxDepth, bool bothSides = false);

}
