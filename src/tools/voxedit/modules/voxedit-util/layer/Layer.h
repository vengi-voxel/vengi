/**
 * @file
 */

#pragma once

#include "voxelrender/RawVolumeRenderer.h"
#include "core/collection/StringMap.h"
#include "core/collection/Array.h"

namespace voxedit {

using LayerMetadata = core::StringMap<core::String>;

struct Layer {
	core::String name;
	glm::ivec3 pivot { 0 };
	bool visible = true;
	bool valid = false;
	bool locked = false;
	LayerMetadata metadata;

	const core::String& metadataById(const core::String& id) const {
		static core::String EMPTY;
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
using Layers = core::Array<Layer, voxelrender::RawVolumeRenderer::MAX_VOLUMES>;

}

