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
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	const int max = (int)materialColors.size();
	for (int i = 0; i < max; ++i) {
		if (!glm::all(glm::epsilonEqual(materialColors[i], color, 0.0001f))) {
			continue;
		}
		return (VoxelType)i;
	}
	Log::error("Could not find any matching voxeltype for color: %s", glm::to_string(color * 255.0f).c_str());
	return VoxelType::Air;
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
