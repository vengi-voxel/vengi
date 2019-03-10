/**
 * @file
 */

#include "ImportHeightmap.h"
#include "voxel/MaterialColor.h"
#include "voxel/polyvox/Region.h"
#include "core/Assert.h"

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

}
