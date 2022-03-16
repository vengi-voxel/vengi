/**
 * @file
 */

#include "ImageUtils.h"
#include "app/App.h"
#include "core/StringUtil.h"
#include "io/FileStream.h"
#include "io/FormatDescription.h"
#include "io/Filesystem.h"
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

bool importPalette(const core::String& file, voxel::Palette &palette) {
	const core::String& ext = core::string::extractExtension(file);
	core::String paletteName(core::string::extractFilename(file.c_str()));
	core::string::replaceAllChars(paletteName, ' ', '_');
	paletteName = paletteName.toLower();
	bool paletteLoaded = false;
	for (const io::FormatDescription* desc = io::format::images(); desc->name != nullptr; ++desc) {
		if (ext == desc->ext) {
			const image::ImagePtr &img = image::loadImage(file, false);
			if (!img->isLoaded()) {
				Log::warn("Failed to load image %s", file.c_str());
				break;
			}
			if (!voxel::Palette::createPalette(img, palette)) {
				Log::warn("Failed to create palette for image %s", file.c_str());
				return false;
			}
			paletteLoaded = true;
			break;
		}
	}
	const io::FilesystemPtr& fs = io::filesystem();
	if (!paletteLoaded) {
		const io::FilePtr& palFile = fs->open(file);
		if (!palFile->validHandle()) {
			Log::warn("Failed to load palette from %s", file.c_str());
			return false;
		}
		io::FileStream stream(palFile);
		if (voxelformat::loadPalette(file, stream, palette) <= 0) {
			Log::warn("Failed to load palette from %s", file.c_str());
			return false;
		}
		paletteLoaded = true;
	}
	return paletteLoaded;
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
	if (imageWidth * imageHeight * thickness > 1024 * 1024 * 4) {
		Log::warn("Did not import plane - max volume size of 1024x1024 (thickness 4) exceeded (%i:%i:%i)", imageWidth, imageHeight, thickness);
		return nullptr;
	}
	Log::info("Import image as plane: w(%i), h(%i), d(%i)", imageWidth, imageHeight, thickness);
	const voxel::Region region(0, 0, 0, imageWidth - 1, imageHeight - 1, thickness - 1);
	const voxel::Palette &palette = voxel::getPalette();
	core::DynamicArray<glm::vec4> materialColors;
	palette.toVec4f(materialColors);
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
				volume->setVoxel(x, (imageHeight - 1) - y, z, voxel);
			}
		}
	}
	return volume;
}

}
