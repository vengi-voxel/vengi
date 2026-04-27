/**
 * @file
 */

#pragma once

#include "core/Optional.h"
#include "core/String.h"
#include "core/Var.h"
#include "palette/Palette.h"
#include "voxedit-ui/ViewMode.h"
#include "voxedit-util/Config.h"
#include "voxel/Region.h"
#include <glm/vec3.hpp>

namespace voxedit {

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
		if (core::getVar(cfg::VoxEditViewMode)->intVal() == (int)ViewMode::AceOfSpades) {
			size = glm::ivec3(512, 64, 512);
		} else {
			size = glm::ivec3(32);
		}
		parent = 0;
	}

	void checkMaxVoxels() {
		int64_t volumeVoxels = (int64_t)size.x * (int64_t)size.y * (int64_t)size.z;
		const int64_t maxVolumeSize = core::getVar(cfg::VoxEditMaxSuggestedVolumeSize)->intVal();
		int64_t maxVoxels = (int64_t)maxVolumeSize * (int64_t)maxVolumeSize * (int64_t)maxVolumeSize;
		if (volumeVoxels > maxVoxels) {
			for (int i = 0; i < 3; ++i) {
				if (size[i] > maxVolumeSize) {
					size[i] = maxVolumeSize;
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
