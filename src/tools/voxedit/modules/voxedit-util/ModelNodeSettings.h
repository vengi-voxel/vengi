/**
 * @file
 */

#pragma once

#include "core/Optional.h"
#include "core/String.h"
#include "palette/Palette.h"
#include "voxel/Region.h"
#include <glm/vec3.hpp>

namespace voxedit {

// you can import models with a bigger volume size - but you can't create them from within the editor
static constexpr const int MaxVolumeSize = 256;

/**
 * @brief A struct that holds the information that are needed when creating new model nodes
 */
struct ModelNodeSettings {
	core::String name;
	core::Optional<palette::Palette> palette;
	glm::ivec3 position;
	glm::ivec3 size;
	int parent = 0;

	ModelNodeSettings() {
		reset();
	}

	inline void reset() {
		position = glm::ivec3(0);
		size = glm::ivec3(32);
		parent = 0;
	}

	void checkMaxVoxels() {
		if (size[0] * size[1] * size[2] > MaxVolumeSize * MaxVolumeSize * MaxVolumeSize) {
			for (int i = 0; i < 3; ++i) {
				if (size[i] > MaxVolumeSize) {
					size[i] = MaxVolumeSize;
				}
			}
		}
	}

	inline voxel::Region region() {
		checkMaxVoxels();
		const voxel::Region region(position, position + size - 1);
		if (!region.isValid()) {
			reset();
			return voxel::Region{position, position + size - 1};
		}
		return region;
	}
};

} // namespace voxedit
