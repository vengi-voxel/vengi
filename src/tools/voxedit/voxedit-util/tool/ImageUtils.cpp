/**
 * @file
 */

#include "ImageUtils.h"
#include "voxel/MaterialColor.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/Voxel.h"
#include "voxel/polyvox/RawVolume.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/GLM.h"
#include <unordered_set>

namespace voxedit {

void importHeightmap(voxel::RawVolumeWrapper& volume, const image::ImagePtr& image) {
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	const voxel::Region& region = volume.region();
	const int volumeHeight = region.getHeightInVoxels();
	const int volumeWidth = region.getWidthInVoxels();
	const int volumeDepth = region.getDepthInVoxels();
	const glm::ivec3& mins = region.getLowerCorner();
	const float stepWidthY = imageHeight / (float)volumeDepth;
	const float stepWidthX = imageWidth / (float)volumeWidth;
	const float scaleHeight = volumeHeight / 255.0f;
	float imageY = 0.0f;
	for (int z = 0; z < volumeDepth; ++z, imageY += stepWidthY) {
		float imageX = 0.0f;
		for (int x = 0; x < volumeWidth; ++x, imageX += stepWidthX) {
			const intptr_t offset = ((int)imageY * imageWidth + (int)imageX) * image->depth();
			const uint8_t* heightmapPixel = image->data() + offset;
			const uint8_t pixelValue = (uint8_t)((float)(*heightmapPixel) * scaleHeight);
			core_assert_msg(offset < imageWidth * imageHeight * image->depth(),
					"Offset %i exceeds valid image data boundaries (w: %i, h: %i, bpp: %i)",
					(int)offset,imageWidth, imageHeight, image->depth());

			for (int y = 0; y < volumeHeight; ++y) {
				const glm::ivec3 pos(x, y, z);
				const glm::ivec3 regionPos = mins + pos;
				if (!region.containsPoint(regionPos)) {
					continue;
				}
				voxel::Voxel voxel;
				if (y < pixelValue) {
					voxel = voxel::createRandomColorVoxel(voxel::VoxelType::Dirt);
				} else if (y == pixelValue) {
					voxel = voxel::createRandomColorVoxel(voxel::VoxelType::Grass);
				}
				volume.setVoxel(regionPos, voxel);
			}
		}
	}
}

voxel::RawVolume* importAsPlane(const image::ImagePtr& image, uint8_t thickness) {
	if (thickness <= 0) {
		return nullptr;
	}
	if (!image || !image->isLoaded()) {
		return nullptr;
	}
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	Log::info("Import image as plane: w(%i), h(%i), d(%i)", imageWidth, imageHeight, thickness);
	const voxel::Region region(0, 0, 0, imageWidth - 1, imageHeight - 1, thickness - 1);
	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	for (int x = 0; x < imageWidth; ++x) {
		for (int y = 0; y < imageHeight; ++y) {
			const uint8_t* data = image->at(x, y);
			const glm::vec4& color = core::Color::fromRGBA(data[0], data[1], data[2], data[3]);
			if (data[3] == 0) {
				continue;
			}
			const uint8_t index = core::Color::getClosestMatch(color, materialColors);
			const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
			for (int z = 0; z < thickness; ++z) {
				volume->setVoxel((imageWidth - 1) - x, (imageHeight - 1) - y, z, voxel);
			}
		}
	}
	return volume;
}

}
