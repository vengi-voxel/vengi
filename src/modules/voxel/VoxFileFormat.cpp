#include "VoxFileFormat.h"
#include "MaterialColor.h"
#include "core/Log.h"
#include <limits>

namespace voxel {

VoxelType VoxFileFormat::findVoxelType(const glm::vec4& color) const {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	const int min = std::enum_value(VoxelType::Min) + 1;
	const int max = std::enum_value(VoxelType::Max);
	core_assert(max <= (int)materialColors.size());
	for (int i = min; i < max; ++i) {
		if (!glm::all(glm::epsilonEqual(materialColors[i], color, 0.0001f))) {
			continue;
		}
		return (VoxelType)i;
	}
	Log::error("Could not find any matching voxeltype for color: %s", glm::to_string(color * 255.0f).c_str());
	return VoxelType::Invalid;
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
