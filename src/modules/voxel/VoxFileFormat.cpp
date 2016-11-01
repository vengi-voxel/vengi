#include "VoxFileFormat.h"
#include "MaterialColor.h"

namespace voxel {

VoxelType VoxFileFormat::findVoxelType(const glm::vec4& color) const {
	static const voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	// TODO:
	return VoxelType::Grass1;
}

glm::vec4 VoxFileFormat::paletteColor(uint32_t index) {
	// TODO:
	if ( false && (_paletteSize <= 0 || index >= _paletteSize)) {
		static const voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
		return materialColors.front();
	}
	//return _palette[index];
	return core::Color::Black;
}

glm::vec4 VoxFileFormat::findClosestMatch(const glm::vec4& color) const {
	// TODO:
	if (_paletteSize > 0) {
	}
	return color;
}

}
