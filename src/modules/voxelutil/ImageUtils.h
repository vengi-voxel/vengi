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
} // namespace voxel

namespace palette {
class PaletteLookup;
class Palette;
} // namespace palette

namespace voxelutil {

/**
 * @brief Import a heightmap with rgb being the surface color and alpha channel being the height
 */
void importColoredHeightmap(voxel::RawVolumeWrapper& volume, const palette::Palette &palette, const image::ImagePtr& image, const voxel::Voxel &underground, uint8_t minHeight = 0, bool adoptHeight = true);
void importHeightmap(voxel::RawVolumeWrapper& volume, const image::ImagePtr& image, const voxel::Voxel &underground, const voxel::Voxel &surface, uint8_t minHeight = 0, bool adoptHeight = true);
int getHeightValueFromAlpha(uint8_t alpha, bool adoptHeight, int volumeHeight, int minHeight);
/**
 * @param alphaAsHeight If this is @c true, the rgb color is used for the colors - otherwise the red channel is used
 * (because a gray scale image is expected)
 */
int importHeightMaxHeight(const image::ImagePtr &image, bool alphaAsHeight);
[[nodiscard]] voxel::RawVolume* importAsPlane(const image::ImagePtr& image, const palette::Palette &palette, uint8_t thickness = 1);
[[nodiscard]] voxel::RawVolume* importAsPlane(const image::Image *image, const palette::Palette &palette, uint8_t thickness = 1);
core::String getDefaultDepthMapFile(const core::String &imageName, const core::String &postfix = "-dm");
[[nodiscard]] voxel::RawVolume* importAsVolume(const image::ImagePtr& image, const palette::Palette &palette, uint8_t maxDepth, bool bothSides = false);
[[nodiscard]] voxel::RawVolume* importAsVolume(const image::ImagePtr& image, const image::ImagePtr& depthMap, const palette::Palette &palette, uint8_t maxDepth, bool bothSides = false);
/**
 * @brief Put a pixel perfect render of the volume into an image
 * @note If the given width and height of the resulting image is not equal to the volume dimensions for the given axis, then
 * the image is scaled
 */
[[nodiscard]] image::ImagePtr renderToImage(const voxel::RawVolume *volume, const palette::Palette &palette,
											voxel::FaceNames frontFace = voxel::FaceNames::Front,
											color::RGBA background = {}, int imgW = -1, int imgH = -1,
											bool upScale = true, float depthFactor = 0.0f);

/**
 * @brief Render a face of a volume within a specific region into an image
 * @note This is useful for capturing the surface colors from a selected face area
 * @param[in] volume The volume to render from
 * @param[in] palette The palette to use for color lookups
 * @param[in] region The region to render from
 * @param[in] face The face to render
 * @return An image with the face's surface colors, or an empty image on failure
 */
[[nodiscard]] image::ImagePtr renderFaceToImage(const voxel::RawVolume *volume, const palette::Palette &palette,
												const voxel::Region &region, voxel::FaceNames face);

/**
 * @brief Creates an isometric render image for voxel::RawVolume
 * @sa voxel::RawVolume
 */
[[nodiscard]] image::ImagePtr renderIsometricImage(const voxel::RawVolume *volume, const palette::Palette &palette,
												   voxel::FaceNames frontFace = voxel::FaceNames::Front,
												   color::RGBA background = {}, int imgW = -1, int imgH = -1,
												   bool upScale = true);
} // namespace voxelutil
