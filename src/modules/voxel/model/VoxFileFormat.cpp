#include "VoxFileFormat.h"
#include "voxel/MaterialColor.h"
#include "core/Log.h"
#include "core/Color.h"
#include <limits>

namespace voxel {

const glm::vec4& VoxFileFormat::getColor(VoxelType type) const {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	return materialColors[std::enum_value(type)];
}

const glm::vec4& VoxFileFormat::getColor(const Voxel& voxel) const {
	return getColor(voxel.getMaterial());
}

VoxelType VoxFileFormat::findVoxelType(const glm::vec4& color) const {
	return voxel::getVoxelType(color);
}

glm::vec4 VoxFileFormat::paletteColor(uint32_t index) const {
	if (index >= _paletteSize) {
		return voxel::getMaterialColors()[1];
	}
	return _palette[index];
}

glm::vec4 VoxFileFormat::findClosestMatch(const glm::vec4& color) const {
	if (_paletteSize > 0) {
		const int index = core::Color::getClosestMatch(color, _palette);
		return paletteColor(index);
	}
	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	materialColors.erase(materialColors.begin());
	const int index = core::Color::getClosestMatch(color, materialColors);
	return materialColors[index];
}

}
