/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/Face.h"
#include <stdint.h>

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
 * @brief Options for @c renderToImage()
 */
struct ImageRenderConfig {
	voxel::FaceNames frontFace = voxel::FaceNames::Front;
	color::RGBA background = {};
	int imgW = -1;
	int imgH = -1;
	bool upScale = true;
	float depthFactor = 0.0f;

	static ImageRenderConfig fromCvars();
	inline uint64_t hash() const {
		const uint32_t flags = (upScale ? 1u : 0u) | ((uint32_t)frontFace << 8);
		const uint32_t depthMilli = (uint32_t)(depthFactor * 1000.0f);
		return (uint64_t)flags ^ ((uint64_t)(uint32_t)imgW << 16) ^ ((uint64_t)(uint32_t)imgH << 32) ^
			   background.rgba ^ ((uint64_t)depthMilli << 40);
	}
};

/**
 * @brief Put a pixel perfect render of the volume into an image
 * @note If the given width and height of the resulting image is not equal to the volume dimensions for the given axis, then
 * the image is scaled
 * @note The default config is @c ImageRenderConfig::fromCvars()
 */
[[nodiscard]] image::ImagePtr renderToImage(const voxel::RawVolume *volume, const palette::Palette &palette,
											const ImageRenderConfig &config = ImageRenderConfig::fromCvars());

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
 * @brief Options for @c renderIsometricImage()
 */
struct IsoRenderConfig {
	voxel::FaceNames frontFace = voxel::FaceNames::Front;
	color::RGBA background = {};
	int imgW = -1;
	int imgH = -1;
	bool upScale = true;
	bool shading = true;
	bool materials = true;
	bool transparency = true;

	static IsoRenderConfig fromCvars();
	inline uint64_t hash() const {
		const uint32_t flags = (shading ? 1u : 0u) | (materials ? 2u : 0u) | (transparency ? 4u : 0u) |
							   (upScale ? 8u : 0u) | ((uint32_t)frontFace << 8);
		return (uint64_t)flags ^ ((uint64_t)(uint32_t)imgW << 16) ^ ((uint64_t)(uint32_t)imgH << 32) ^ background.rgba;
	}
};

/**
 * @brief Creates a 2:1 dimetric render of a voxel::RawVolume
 *
 * Each voxel is an 8x7 cube stamp. Visible faces use Lambert shading in linear
 * RGB. Palette materials (emit, metal, roughness) are applied when @c materials
 * is set. Palette alpha is composited when @c transparency is set.
 * @note The default config is @c IsoRenderConfig::fromCvars()
 * @see https://pikuma.com/blog/isometric-projection-in-games
 * @see https://www.slynyrd.com/blog/2022/11/28/pixelblog-41-isometric-pixel-art
 * @sa voxel::RawVolume
 */
[[nodiscard]] image::ImagePtr renderIsometricImage(const voxel::RawVolume *volume, const palette::Palette &palette,
												   const IsoRenderConfig &config = IsoRenderConfig::fromCvars());
} // namespace voxelutil
