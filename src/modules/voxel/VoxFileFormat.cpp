#include "VoxFileFormat.h"
#include "MaterialColor.h"
#include "core/Log.h"
#include <limits>

namespace voxel {

VoxelType VoxFileFormat::findVoxelType(const glm::vec4& color) const {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	const int min = std::enum_value(VoxelType::Min);
	const int max = std::enum_value(VoxelType::Max);
	for (int i = min; i < max; ++i) {
		if (glm::all(glm::epsilonEqual(materialColors[i], color, 0.0001f))) {
			return (VoxelType)i;
		}
	}
	Log::error("Could not find any matching voxeltype for color: %s", glm::to_string(color).c_str());
	return VoxelType::Invalid;
}

glm::vec4 VoxFileFormat::paletteColor(uint32_t index) {
	if (index >= _paletteSize) {
		return core::Color::Black;
	}
	return _palette[index];
}

glm::vec4 VoxFileFormat::findClosestMatch(const glm::vec4& color) const {
	if (_paletteSize > 0) {
	}
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	const int index = core::Color::getClosestMatch(color, materialColors);
	return materialColors[index];
}

}
