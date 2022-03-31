/**
 * @file
 */

#include "ImageUtils.h"
#include "core/StringUtil.h"
#include "io/FileStream.h"
#include "io/FormatDescription.h"
#include "voxel/MaterialColor.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelutil {

void importHeightmap(voxel::RawVolumeWrapper& volume, const image::ImagePtr& image, const voxel::Voxel &underground, const voxel::Voxel &surface) {
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	const voxel::Region& region = volume.region();
	const int volumeHeight = region.getHeightInVoxels();
	const int volumeWidth = region.getWidthInVoxels();
	const int volumeDepth = region.getDepthInVoxels();
	const glm::ivec3& mins = region.getLowerCorner();
	const float stepWidthY = (float)imageHeight / (float)volumeDepth;
	const float stepWidthX = (float)imageWidth / (float)volumeWidth;
	const float scaleHeight = (float)volumeHeight / 255.0f;
	float imageY = 0.0f;
	for (int z = 0; z < volumeDepth; ++z, imageY += stepWidthY) {
		float imageX = 0.0f;
		for (int x = 0; x < volumeWidth; ++x, imageX += stepWidthX) {
			const uint8_t* heightmapPixel = image->at((int)imageX, (int)imageY);
			const uint8_t pixelValue = (uint8_t)((float)(*heightmapPixel) * scaleHeight);

			for (int y = 0; y < volumeHeight; ++y) {
				const glm::ivec3 pos(x, y, z);
				const glm::ivec3 regionPos = mins + pos;
				if (!region.containsPoint(regionPos)) {
					continue;
				}
				voxel::Voxel voxel;
				if (y < pixelValue) {
					voxel = underground;
				} else if (y == pixelValue) {
					voxel = surface;
				}
				volume.setVoxel(regionPos, voxel);
			}
		}
	}
}

voxel::RawVolume* importAsPlane(const image::ImagePtr& image, uint8_t thickness) {
	if (thickness <= 0) {
		Log::error("Thickness can't be 0");
		return nullptr;
	}
	if (!image || !image->isLoaded()) {
		Log::error("No color image given");
		return nullptr;
	}
	if (image->depth() != 4) {
		Log::error("Expected to get an rgba image");
		return nullptr;
	}
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	if (imageWidth * imageHeight * thickness > 1024 * 1024 * 4) {
		Log::warn("Did not import plane - max volume size of 1024x1024 (thickness 4) exceeded (%i:%i:%i)", imageWidth, imageHeight, thickness);
		return nullptr;
	}
	Log::info("Import image as plane: w(%i), h(%i), d(%i)", imageWidth, imageHeight, thickness);
	const voxel::Region region(0, 0, 0, imageWidth - 1, imageHeight - 1, thickness - 1);
	const voxel::Palette &palette = voxel::getPalette();
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	for (int x = 0; x < imageWidth; ++x) {
		for (int y = 0; y < imageHeight; ++y) {
			const uint8_t* data = image->at(x, y);
			const glm::vec4& color = core::Color::fromRGBA(data[0], data[1], data[2], data[3]);
			if (data[3] == 0) {
				continue;
			}
			const uint8_t index = palette.getClosestMatch(color);
			const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
			for (int z = 0; z < thickness; ++z) {
				volume->setVoxel(x, (imageHeight - 1) - y, z, voxel);
			}
		}
	}
	return volume;
}

voxel::RawVolume* importAsVolume(const image::ImagePtr& image, const image::ImagePtr& heightmap, uint8_t maxDepth, bool bothSides) {
	if (maxDepth <= 0) {
		Log::error("Max height can't be 0");
		return nullptr;
	}
	if (!image || !image->isLoaded()) {
		Log::error("No color image given");
		return nullptr;
	}
	if (!heightmap || !heightmap->isLoaded()) {
		Log::error("No heightmap given");
		return nullptr;
	}
	if (heightmap->width() != image->width() || heightmap->height() != image->height()) {
		Log::error("Image dimensions differ for color and heigtmap");
		return nullptr;
	}
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	int volumeDepth = bothSides ? maxDepth * 2 : maxDepth;
	if (volumeDepth % 2 == 0) {
		Log::warn("Make max volume depth uneven");
		volumeDepth++;
	}
	if (imageWidth * imageHeight * volumeDepth > 1024 * 1024 * 4) {
		Log::warn("Did not import plane - max volume size of 1024x1024 (depth 4) exceeded (%i:%i:%i)", imageWidth, imageHeight, volumeDepth);
		return nullptr;
	}
	Log::info("Import image as volume: w(%i), h(%i), d(%i)", imageWidth, imageHeight, volumeDepth);
	const voxel::Region region(0, 0, 0, imageWidth - 1, imageHeight - 1, volumeDepth - 1);
	const voxel::Palette &palette = voxel::getPalette();
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	for (int x = 0; x < imageWidth; ++x) {
		for (int y = 0; y < imageHeight; ++y) {
			const uint8_t* data = image->at(x, y);
			if (data[3] == 0) {
				continue;
			}
			const glm::vec4& color = core::Color::fromRGBA(data[0], data[1], data[2], data[3]);
			const uint8_t index = palette.getClosestMatch(color);
			const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
			const uint8_t* heightdata = heightmap->at(x, y);
			const float thickness = (float)*heightdata;
			const float maxthickness = maxDepth;
			const float height = thickness * maxthickness / 255.0f;
			if (bothSides) {
				const int heighti = (int)glm::ceil(height/ 2.0f);
				const int minZ = maxDepth - heighti;
				const int maxZ = maxDepth + heighti;
				for (int z = minZ; z <= maxZ; ++z) {
					volume->setVoxel(x, (imageHeight - 1) - y, z, voxel);
				}
			} else {
				const int heighti = (int)glm::ceil(height);
				for (int z = 0; z < heighti; ++z) {
					volume->setVoxel(x, (imageHeight - 1) - y, z, voxel);
				}
			}
		}
	}
	return volume;
}

}
