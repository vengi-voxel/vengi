/**
 * @file
 */

#include "ImageUtils.h"
#include "app/Async.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelutil {

bool importFace(voxel::RawVolumeWrapper &volume, const voxel::Region &region, const palette::Palette &palette, voxel::FaceNames faceName,
				const image::ImagePtr &image, const glm::vec2 &uv0, const glm::vec2 &uv1, uint8_t replacementPalIdx) {
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();
	const math::Axis axis = faceToAxis(faceName);
	const int axisIdx0 = math::getIndexForAxis(axis);
	const int axisIdx1 = axis == math::Axis::Y ? (axisIdx0 + 2) % 3 : (axisIdx0 + 1) % 3;
	const int axisIdx2 = axis == math::Axis::Y ? (axisIdx0 + 1) % 3 : (axisIdx0 + 2) % 3;
	const glm::vec3 size = region.getDimensionsInVoxels();
	const bool negativeFace = voxel::isNegativeFace(faceName);

	const int axisFixed = negativeFace ? mins[axisIdx0] : maxs[axisIdx0];
	const int axisMins1 = mins[axisIdx1];
	const int axisMins2 = mins[axisIdx2];
	const int axisMaxs1 = maxs[axisIdx1];
	const int axisMaxs2 = maxs[axisIdx2];
	const int axisIdxUV1 = (axisIdx1 + 0) % 2;
	const int axisIdxUV2 = (axisIdx1 + 1) % 2;

	app::for_parallel(axisMins1, axisMaxs1 + 1, [axisFixed, axisIdx0, axisIdx1, axisIdx2, axisMaxs2, axisMins2, axisIdxUV1, axisIdxUV2, replacementPalIdx, &image, &palette, &uv0, &uv1, &size, &volume, axisMins1] (int start, int end) {
		const bool flipU = false;
		const bool flipV = false;
		const image::TextureWrap wrapS = flipU ? image::TextureWrap::MirroredRepeat : image::Repeat;
		const image::TextureWrap wrapT = flipV ? image::TextureWrap::MirroredRepeat : image::Repeat;
		for (int axis1 = start; axis1 < end; ++axis1) {
			const float axis1Factor = ((float)(axis1 - axisMins1) + 0.5f) / (float)size[axisIdx1];
			for (int axis2 = axisMins2; axis2 <= axisMaxs2; ++axis2) {
				int palIdx = replacementPalIdx;
				if (image) {
					const float axis2Factor = ((float)(axis2 - axisMins2) + 0.5f) / (float)size[axisIdx2];
					glm::vec2 uv;
					uv[axisIdxUV1] = glm::mix(flipU ? -uv0[axisIdxUV1] : uv0[axisIdxUV1],
											flipV ? -uv1[axisIdxUV1] : uv1[axisIdxUV1], axis1Factor);
					uv[axisIdxUV2] = glm::mix(flipU ? -uv0[axisIdxUV2] : uv0[axisIdxUV2],
											flipV ? -uv1[axisIdxUV2] : uv1[axisIdxUV2], axis2Factor);
					const core::RGBA color = image->colorAt(uv, wrapS, wrapT);
					if (color.a == 0) {
						continue;
					}
					palIdx = palette.getClosestMatch(color);
					if (palIdx == palette::PaletteColorNotFound) {
						palIdx = replacementPalIdx;
					}
				}
				glm::ivec3 pos;
				pos[axisIdx0] = axisFixed;
				pos[axisIdx1] = axis1;
				pos[axisIdx2] = axis2;
				volume.setVoxel(pos, voxel::createVoxel(palette, palIdx));
			}
		}
	});
	return true;
}

int importHeightMaxHeight(const image::ImagePtr &image, bool alphaAsHeight) {
	const int w = image->width();
	const int h = image->height();
	int maxHeight = 0;
	int minHeight = 255;
	if (alphaAsHeight) {
		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				const uint8_t alphaVal = image->colorAt(x, y).a;
				maxHeight = core_max(maxHeight, alphaVal);
				minHeight = core_min(minHeight, alphaVal);
			}
		}
	} else {
		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				const uint8_t redVal = image->colorAt(x, y).r;
				maxHeight = core_max(maxHeight, redVal);
				minHeight = core_min(minHeight, redVal);
			}
		}
	}
	if (maxHeight == minHeight) {
		return 1;
	}
	return maxHeight;
}

int getHeightValueFromAlpha(uint8_t alpha, bool adoptHeight, int volumeHeight, int minHeight) {
	int heightValue = adoptHeight ? (int)glm::round((float)alpha * (float)volumeHeight / 255.0f) : (int)alpha;
	if (heightValue < minHeight) {
		heightValue = minHeight;
	}
	return heightValue;
}

void importColoredHeightmap(voxel::RawVolumeWrapper &volume, const palette::Palette &palette,
							const image::ImagePtr &image, const voxel::Voxel &underground, uint8_t minHeight,
							bool adoptHeight) {
	auto fn = [&palette, &volume, &image, adoptHeight, minHeight, underground](int start, int end) {
		const voxel::Region &region = volume.region();
		const int volumeHeight = region.getHeightInVoxels();
		const int volumeWidth = region.getWidthInVoxels();
		const int volumeDepth = region.getDepthInVoxels();
		const glm::ivec3 &mins = region.getLowerCorner();
		const int imageWidth = image->width();
		const int imageHeight = image->height();
		const float stepWidthY = (float)imageHeight / (float)volumeDepth;
		const float stepWidthX = (float)imageWidth / (float)volumeWidth;
		palette::PaletteLookup palLookup(palette);
		float imageY = start * stepWidthY;

		voxel::RawVolumeWrapper::Sampler sampler(volume);
		for (int z = start; z < end; ++z, imageY += stepWidthY) {
			float imageX = 0.0f;
			for (int x = 0; x < volumeWidth; ++x, imageX += stepWidthX) {
				const core::RGBA heightmapPixel = image->colorAt((int)imageX, (int)imageY);
				const uint8_t heightValue =
					getHeightValueFromAlpha(heightmapPixel.a, adoptHeight, volumeHeight, minHeight);
				const core::RGBA rgba(heightmapPixel.r, heightmapPixel.g, heightmapPixel.b);
				const uint8_t palidx = palLookup.findClosestIndex(rgba);
				const voxel::Voxel surfaceVoxel = voxel::createVoxel(palLookup.palette(), palidx);
				if (voxel::isAir(underground.getMaterial())) {
					const glm::ivec3 pos(x, heightValue - 1, z);
					const glm::ivec3 regionPos = mins + pos;
					sampler.setPosition(regionPos);
					sampler.setVoxel(surfaceVoxel);
				} else {
					const glm::ivec3 pos(x, 0, z);
					const glm::ivec3 regionPos = mins + pos;
					sampler.setPosition(regionPos);

					for (int y = 0; y < heightValue; ++y) {
						voxel::Voxel voxel = underground;
						if (y >= heightValue - 1) {
							voxel = surfaceVoxel;
						}
						sampler.setVoxel(voxel);
						sampler.movePositiveY();
					}
				}
			}
		}
	};
	app::for_parallel(0, volume.region().getDepthInVoxels(), fn);
}

void importHeightmap(voxel::RawVolumeWrapper &volume, const image::ImagePtr &image, const voxel::Voxel &underground,
					 const voxel::Voxel &surface, uint8_t minHeight, bool adoptHeight) {
	const int maxImageHeight = importHeightMaxHeight(image, true);
	app::for_parallel(0, volume.region().getDepthInVoxels(), [&image, adoptHeight, minHeight, &volume, underground, surface, maxImageHeight] (int start, int end) {
		const int imageWidth = image->width();
		const int imageHeight = image->height();
		const voxel::Region &region = volume.region();
		const int volumeHeight = region.getHeightInVoxels();
		const int volumeWidth = region.getWidthInVoxels();
		const int volumeDepth = region.getDepthInVoxels();
		const glm::ivec3 &mins = region.getLowerCorner();
		const float stepWidthY = (float)imageHeight / (float)volumeDepth;
		const float stepWidthX = (float)imageWidth / (float)volumeWidth;
		Log::debug("stepwidth: %f %f", stepWidthX, stepWidthY);
		const float scaleHeight = adoptHeight ? (float)volumeHeight / (float)maxImageHeight : 1.0f;
		float imageY = 0.0f;
		voxel::RawVolumeWrapper::Sampler sampler(volume);
		sampler.setPosition(mins.x, mins.y, mins.z + start);
		for (int z = start; z < end; ++z, imageY += stepWidthY) {
			float imageX = 0.0f;
			voxel::RawVolumeWrapper::Sampler sampler2 = sampler;
			for (int x = 0; x < volumeWidth; ++x, imageX += stepWidthX) {
				voxel::RawVolumeWrapper::Sampler sampler3 = sampler2;
				const core::RGBA heightmapPixel = image->colorAt((int)imageX, (int)imageY);
				uint8_t heightValue = (uint8_t)(glm::round((float)(heightmapPixel.r) * scaleHeight));
				if (heightValue < minHeight) {
					heightValue = minHeight;
				}

				if (voxel::isAir(underground.getMaterial())) {
					sampler3.movePositiveY(heightValue - 1);
					sampler.setVoxel(surface);
				} else {
					for (int y = 0; y < heightValue; ++y) {
						voxel::Voxel voxel = surface;
						if (y < heightValue - 1) {
							voxel = underground;
						}
						sampler3.setVoxel(voxel);
						sampler3.movePositiveY();
					}
				}
				sampler2.movePositiveX();
			}
			sampler.movePositiveZ();
		}
	});
}

voxel::RawVolume *importAsPlane(const image::ImagePtr &image, uint8_t thickness) {
	return importAsPlane(image.get(), thickness);
}

voxel::RawVolume *importAsPlane(const image::Image *image, uint8_t thickness) {
	const palette::Palette &palette = voxel::getPalette();
	return importAsPlane(image, palette, thickness);
}

voxel::RawVolume *importAsPlane(const image::ImagePtr &image, const palette::Palette &palette, uint8_t thickness) {
	return importAsPlane(image.get(), palette, thickness);
}

voxel::RawVolume *importAsPlane(const image::Image *image, const palette::Palette &palette, uint8_t thickness) {
	if (thickness <= 0) {
		Log::error("Thickness can't be 0");
		return nullptr;
	}
	if (!image || !image->isLoaded()) {
		Log::error("No color image given");
		return nullptr;
	}
	if (image->components() != 4) {
		Log::error("Expected to get an rgba image");
		return nullptr;
	}
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	if (imageWidth * imageHeight * thickness > 1024 * 1024 * 256) {
		Log::warn("Did not import plane - max volume size of 1024x1024 (thickness 256) exceeded (%i:%i:%i)", imageWidth,
				  imageHeight, thickness);
		return nullptr;
	}
	Log::debug("Import image as plane: w(%i), h(%i), d(%i)", imageWidth, imageHeight, thickness);
	const voxel::Region region(0, 0, 0, imageWidth - 1, imageHeight - 1, thickness - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);

	app::for_parallel(0, imageWidth, [&image, &palette, &volume, imageHeight, thickness] (int start, int end) {
		palette::PaletteLookup palLookup(palette);
		voxel::RawVolume::Sampler sampler(volume);
		sampler.setPosition(start, imageHeight - 1, 0);
		for (int x = start; x < end; ++x) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int y = 0; y < imageHeight; ++y) {
				const core::RGBA data = image->colorAt(x, y);
				if (data.a == 0) {
					continue;
				}
				const uint8_t index = palLookup.findClosestIndex(data);
				const voxel::Voxel voxel = voxel::createVoxel(palette, index);
				voxel::RawVolume::Sampler sampler3 = sampler2;
				for (int z = 0; z < thickness; ++z) {
					sampler3.setVoxel(voxel);
					sampler3.movePositiveZ();
				}
				sampler2.moveNegativeY();
			}
			sampler.movePositiveX();
		}
	});
	return volume;
}

voxel::RawVolume *importAsVolume(const image::ImagePtr &image, uint8_t maxDepth, bool bothSides) {
	return importAsVolume(image, voxel::getPalette(), maxDepth, bothSides);
}

core::String getDefaultDepthMapFile(const core::String &imageName, const core::String &postfix) {
	return core::string::addPostfixToFile(imageName, postfix);
}

voxel::RawVolume *importAsVolume(const image::ImagePtr &image, const palette::Palette &palette, uint8_t maxDepth,
								 bool bothSides) {
	if (maxDepth <= 0) {
		Log::error("Max height can't be 0");
		return nullptr;
	}
	if (!image || !image->isLoaded()) {
		Log::error("No color image given");
		return nullptr;
	}
	const core::String &dmFile = getDefaultDepthMapFile(image->name());
	const image::ImagePtr &depthMap = image::loadImage(dmFile);
	if (!depthMap || !depthMap->isLoaded()) {
		Log::error("Couldn't load depthmap %s", dmFile.c_str());
		return nullptr;
	}
	return importAsVolume(image, depthMap, palette, maxDepth, bothSides);
}

voxel::RawVolume *importAsVolume(const image::ImagePtr &image, const image::ImagePtr &depthmap,
								 const palette::Palette &palette, uint8_t maxDepth, bool bothSides) {
	core_assert(image);
	core_assert(depthmap);
	if (!image->isLoaded()) {
		Log::error("Image '%s' is not loaded", image->name().c_str());
		return nullptr;
	}
	if (!depthmap->isLoaded()) {
		Log::error("Depthmap '%s' is not loaded", image->name().c_str());
		return nullptr;
	}

	if (depthmap->width() != image->width() || depthmap->height() != image->height()) {
		Log::error("Image dimensions differ for color and depthmap");
		return nullptr;
	}
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	int volumeDepth = bothSides ? maxDepth * 2 + 1 : maxDepth;
	if (volumeDepth % 2 == 0) {
		Log::warn("Make max volume depth uneven");
		volumeDepth++;
	}
	if (imageWidth * imageHeight * volumeDepth > 1024 * 1024 * 256) {
		Log::warn("Did not import plane - max volume size of 1024x1024 (depth 256) exceeded (%i:%i:%i)", imageWidth,
				  imageHeight, volumeDepth);
		return nullptr;
	}
	Log::debug("Import image as volume: w(%i), h(%i), d(%i)", imageWidth, imageHeight, volumeDepth);
	const voxel::Region region(0, 0, 0, imageWidth - 1, imageHeight - 1, volumeDepth - 1);
	palette::PaletteLookup palLookup(palette);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	voxel::RawVolumeWrapper wrapper(volume);
	// TODO: PERF: FOR_PARALLEL
	// TODO: PERF: use a volume sampler
	for (int x = 0; x < imageWidth; ++x) {
		for (int y = 0; y < imageHeight; ++y) {
			const core::RGBA data = image->colorAt(x, y);
			if (data.a == 0) {
				continue;
			}
			const glm::vec4 &color = core::Color::fromRGBA(data);
			const uint8_t index = palLookup.findClosestIndex(color);
			const voxel::Voxel voxel = voxel::createVoxel(palLookup.palette(), index);
			const core::RGBA heightdata = depthmap->colorAt(x, y);
			const float thickness = (float)heightdata.r;
			const float maxthickness = maxDepth;
			const float height = thickness * maxthickness / 255.0f;
			if (bothSides) {
				const int heighti = (int)glm::ceil(height / 2.0f);
				const int minZ = maxDepth - heighti;
				const int maxZ = maxDepth + heighti;
				for (int z = minZ; z <= maxZ; ++z) {
					wrapper.setVoxel(x, region.getUpperY() - y, z, voxel);
				}
			} else {
				const int heighti = (int)glm::ceil(height);
				for (int z = region.getLowerZ(); z < region.getLowerZ() + heighti; ++z) {
					wrapper.setVoxel(x, region.getUpperY() - y, z, voxel);
				}
			}
		}
	}
	return volume;
}

} // namespace voxelutil
