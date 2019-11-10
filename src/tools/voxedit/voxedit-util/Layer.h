/**
 * @file
 */

#pragma once

#include "voxelrender/RawVolumeRenderer.h"
#include <string>
#include <array>

namespace voxedit {

using LayerMetadata = std::unordered_map<std::string, std::string>;

struct Layer {
	std::string name;
	glm::ivec3 pivot { 0 };
	bool visible = true;
	bool valid = false;
	bool locked = false;
	LayerMetadata metadata;

	const std::string& metadataById(const std::string& id) const {
		static std::string EMPTY;
		auto i = metadata.find(id);
		if (i == metadata.end()) {
			return EMPTY;
		}
		return i->second;
	}

	void reset() {
		name.clear();
		visible = true;
		valid = false;
		locked = false;
		pivot = glm::zero<glm::ivec3>();
		metadata.clear();
	}
};
using Layers = std::array<Layer, voxelrender::RawVolumeRenderer::MAX_VOLUMES>;

}

