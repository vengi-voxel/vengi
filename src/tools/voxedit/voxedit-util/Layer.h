/**
 * @file
 */

#pragma once

#include "voxelrender/RawVolumeRenderer.h"
#include <string>
#include <array>

namespace voxedit {

struct Layer {
	std::string name;
	glm::ivec3 pivot { 0 };
	bool visible = true;
	bool valid = false;

	void reset() {
		name.clear();
		visible = true;
		valid = false;
		pivot = glm::zero<glm::ivec3>();
	}
};
using Layers = std::array<Layer, voxelrender::RawVolumeRenderer::MAX_VOLUMES>;

}

