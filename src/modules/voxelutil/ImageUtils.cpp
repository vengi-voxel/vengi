/**
 * @file
 */

#include "ImageUtils.h"
#include "app/Async.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "image/Image.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

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
	palette::PaletteLookup palLookup(palette);
	auto fn = [&palLookup, &volume, &image, adoptHeight, minHeight, underground](int start, int end) {
		const voxel::Region &region = volume.region();
		const int volumeHeight = region.getHeightInVoxels();
		const int volumeWidth = region.getWidthInVoxels();
		const int volumeDepth = region.getDepthInVoxels();
		const glm::ivec3 &mins = region.getLowerCorner();
		const int imageWidth = image->width();
		const int imageHeight = image->height();
		const float stepWidthY = (float)imageHeight / (float)volumeDepth;
		const float stepWidthX = (float)imageWidth / (float)volumeWidth;
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

	palette::PaletteLookup palLookup(palette);
	app::for_parallel(0, imageWidth, [&image, &palLookup, &palette, &volume, imageHeight, thickness] (int start, int end) {
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
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	palette::PaletteLookup palLookup(palette);
	auto fn = [&palLookup, &palette, imageWidth, volume, image, depthmap, maxDepth, bothSides] (int start, int end) {
		voxel::RawVolume::Sampler sampler(volume);
		sampler.setPosition(0, volume->region().getUpperY(), 0);
		for (int y = start; y < end; ++y) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int x = 0; x < imageWidth; ++x) {
				const core::RGBA data = image->colorAt(x, y);
				if (data.a == 0 /* AlphaThreshold */) {
					sampler2.movePositiveX();
					continue;
				}
				const uint8_t index = palLookup.findClosestIndex(data);
				const voxel::Voxel voxel = voxel::createVoxel(palette, index);
				const core::RGBA heightdata = depthmap->colorAt(x, y);
				const float thickness = (float)heightdata.r;
				const float maxthickness = maxDepth;
				const float height = thickness * maxthickness / 255.0f;
				if (bothSides) {
					const int heighti = (int)glm::ceil(height / 2.0f);
					const int minZ = maxDepth - heighti;
					const int maxZ = maxDepth + heighti;
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (int z = minZ; z <= maxZ; ++z) {
						sampler3.setVoxel(voxel);
						sampler3.movePositiveZ();
					}
				} else {
					const int heighti = (int)glm::ceil(height);
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (int z = volume->region().getLowerZ(); z < volume->region().getLowerZ() + heighti; ++z) {
						sampler3.setVoxel(voxel);
						sampler3.movePositiveZ();
					}
				}
				sampler2.movePositiveX();
			}
			sampler.moveNegativeY();
		}
	};
	app::for_parallel(0, imageHeight, fn);
	return volume;
}

namespace {
const int VoxelSpriteWidth = 4;
const int VoxelSpriteHeight = 4;
} // namespace

static void renderIsometricVoxel(const image::ImagePtr &img, const palette::Palette &palette, int x, int y, int z,
								 const voxel::Voxel &v) {
	const core::RGBA palCol = palette.color(v.getColor());
	const core::RGBA darkerCol = core::Color::darker(palCol);
	const core::RGBA lighterCol = core::Color::brighter(palCol);
	const core::RGBA colors[VoxelSpriteWidth] = {darkerCol, darkerCol, lighterCol, lighterCol};
	for (int j = 0; j < VoxelSpriteHeight; ++j) {
		const int py = y + j;
		if (py < 0 || py >= img->height()) {
			continue;
		}
		for (int i = 0; i < VoxelSpriteWidth; ++i) {
			const int px = x + i;
			if (px < 0 || px >= img->width()) {
				continue;
			}
			core::RGBA c;
			if (j == 0) {
				c = lighterCol;
			} else {
				c = colors[i];
			}
			img->setColor(c, px, py);
		}
	}
}

image::ImagePtr renderIsometricImage(const voxel::RawVolume *volume, const palette::Palette &palette,
									 voxel::FaceNames frontFace, core::RGBA background, int imgW, int imgH,
									 bool upScale) {
	const voxel::Region &r = volume->region();

	const int sizeX = r.getWidthInVoxels();
	const int sizeZ = r.getDepthInVoxels();
	const int minY = r.getLowerY();
	const int maxY = r.getUpperY();

	image::ImagePtr image = image::createEmptyImage("isometric");
	image->resize((sizeX + sizeZ) * 2, sizeX + sizeZ + (maxY - minY + 1) * 3 - 1);
	for (int x = 0; x < image->width(); ++x) {
		for (int y = 0; y < image->height(); ++y) {
			image->setColor(background, x, y);
		}
	}

	// TODO: visitor order is not yet working
	VisitorOrder visitorOrder;
	switch (frontFace) {
	case voxel::FaceNames::Front:
		visitorOrder = VisitorOrder::mXmZY;
		break;
	case voxel::FaceNames::Back:
		visitorOrder = VisitorOrder::mXZmY;
		break;
	case voxel::FaceNames::Right:
		visitorOrder = VisitorOrder::mYmZmX;
		break;
	case voxel::FaceNames::Left:
		visitorOrder = VisitorOrder::mYZX;
		break;
	case voxel::FaceNames::Up:
		visitorOrder = VisitorOrder::mZmXmY;
		break;
	case voxel::FaceNames::Down:
		visitorOrder = VisitorOrder::ZmXY;
		break;
	default:
		return 0;
	}
	// visitor to draw each visible voxel. We must translate volume coords to image coords.
	auto func = [&](int vx, int vy, int vz, const voxel::Voxel &v) {
		const int x = vx - r.getLowerX();
		const int z = vz - r.getLowerZ();
		const int bmpPosX = 2 * (sizeZ - 1) + (x - z) * 2;
		const int bmpPosY = image->height() - 2 + x + z - sizeX - sizeZ - (vy - minY) * 3;
		renderIsometricVoxel(image, palette, bmpPosX, bmpPosY, vy, v);
	};
	voxelutil::visitSurfaceVolume(*volume, func, visitorOrder);

	// check if we need to rescale the image
	int width = image->width();
	int height = image->height();
	if ((imgW > 0 && imgW != width) || (imgH > 0 && imgH != height)) {
		if (imgW <= 0) {
			const float factor = (float)imgH / (float)height;
			imgW = (int)glm::round((float)width * factor);
		}
		if (imgH <= 0) {
			const float factor = (float)imgW / (float)width;
			imgH = (int)glm::round((float)height * factor);
		}
		const bool wouldUpscale = imgW > width || imgH > height;
		if (upScale || !wouldUpscale) {
			image->resize(imgW, imgH);
		}
	}

	// finally mark this as loaded to indicate that the image data is valid
	image->markLoaded();

	return image;
}

image::ImagePtr renderToImage(const voxel::RawVolume *volume, const palette::Palette &palette,
							  voxel::FaceNames frontFace, core::RGBA background, int imgW, int imgH, bool upScale,
							  float depthFactor) {
	image::ImagePtr image = core::make_shared<image::Image>("renderToImage");
	const voxel::Region &region = volume->region();
	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	int width = 1;
	int height = 1;
	if (voxel::isY(frontFace)) {
		width = dim.x;
		height = dim.z;
	} else if (voxel::isX(frontFace)) {
		width = dim.z;
		height = dim.y;
	} else if (voxel::isZ(frontFace)) {
		width = dim.x;
		height = dim.y;
	}

	// now that we have the size - fill it with the background color
	image->resize(width, height);
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			image->setColor(background, x, y);
		}
	}

	// now render the voxels to the image
	voxelutil::visitFace(*volume, frontFace, [frontFace, &palette, &image, &region, height, depthFactor] (int x, int y, int z, const voxel::Voxel& voxel) {
		core::RGBA rgba = palette.color(voxel.getColor());
		int px = 0;
		int py = 0;
		float depth = 0.0f;
		if (voxel::isY(frontFace)) {
			px = x - region.getLowerX();
			py = height - 1 - (z - region.getLowerZ());
			if (voxel::isPositiveFace(frontFace)) {
				depth = (float)((region.getUpperY() - y) * depthFactor) / (float)region.getHeightInVoxels();
			} else {
				depth = (float)((y - region.getLowerY()) * depthFactor) / (float)region.getHeightInVoxels();
			}
		} else if (voxel::isX(frontFace)) {
			px = z - region.getLowerZ();
			py = height - 1 - (y - region.getLowerY());
			if (voxel::isPositiveFace(frontFace)) {
				depth = (float)((region.getUpperX() - x) * depthFactor) / (float)region.getWidthInVoxels();
			} else {
				depth = (float)((x - region.getLowerX()) * depthFactor) / (float)region.getWidthInVoxels();
			}

		} else if (voxel::isZ(frontFace)) {
			px = x - region.getLowerX();
			py = height - 1 - (y - region.getLowerY());
			if (voxel::isPositiveFace(frontFace)) {
				depth = (float)((region.getUpperZ() - z) * depthFactor) / (float)region.getDepthInVoxels();
			} else {
				depth = (float)((z - region.getLowerZ()) * depthFactor) / (float)region.getDepthInVoxels();
			}
		} else {
			return;
		}

		if (depthFactor > 0.0f) {
			rgba = core::Color::darker(rgba, depth);
		}
		image->setColor(rgba, px, py);
	}, VisitorOrder::Max, true);

	// check if we need to rescale the image
	if ((imgW > 0 && imgW != width) || (imgH > 0 && imgH != height)) {
		if (imgW <= 0) {
			const float factor = (float)imgH / (float)height;
			imgW = (int)glm::round((float)width * factor);
		}
		if (imgH <= 0) {
			const float factor = (float)imgW / (float)width;
			imgH = (int)glm::round((float)height * factor);
		}
		const bool wouldUpscale = imgW > width || imgH > height;
		if (upScale || !wouldUpscale) {
			image->resize(imgW, imgH);
		}
	}

	// finally mark this as loaded to indicate that the image data is valid
	image->markLoaded();

	return image;
}

} // namespace voxelutil
