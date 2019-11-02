/**
 * @file
 */

#include "VoxFileFormat.h"
#include "voxel/MaterialColor.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Color.h"
#include <limits>

namespace voxel {

const glm::vec4& VoxFileFormat::getColor(const Voxel& voxel) const {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	return materialColors[voxel.getColor()];
}

uint8_t VoxFileFormat::convertPaletteIndex(uint32_t paletteIndex) const {
	if (paletteIndex >= _paletteSize) {
		return 1;
	}
	return _palette[paletteIndex];
}

glm::vec4 VoxFileFormat::findClosestMatch(const glm::vec4& color) const {
	const int index = findClosestIndex(color);
	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	return materialColors[index];
}

uint8_t VoxFileFormat::findClosestIndex(const glm::vec4& color) const {
	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	//materialColors.erase(materialColors.begin());
	return core::Color::getClosestMatch(color, materialColors);
}

RawVolume* VoxFileFormat::merge(const VoxelVolumes& volumes) const {
	return volumes.merge();
}

RawVolume* VoxFileFormat::load(const io::FilePtr& file) {
	VoxelVolumes volumes;
	if (!loadGroups(file, volumes)) {
		return nullptr;
	}
	RawVolume* mergedVolume = merge(volumes);
	for (auto& v : volumes) {
		delete v.volume;
	}
	return mergedVolume;
}

bool VoxFileFormat::save(const RawVolume* volume, const io::FilePtr& file) {
	VoxelVolumes volumes;
	volumes.volumes = {VoxelVolume(const_cast<RawVolume*>(volume))};
	return saveGroups(volumes, file);
}

}
