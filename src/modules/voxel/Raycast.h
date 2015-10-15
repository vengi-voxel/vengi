#pragma once

#include <PolyVox/Raycast.h>
#include <glm/glm.hpp>
#include "WorldData.h"

namespace voxel {

class Raycast {
private:
	glm::vec3 _position;
	voxel::Voxel _voxel;
public:
	inline glm::vec3 position() const {
		return _position;
	}

	inline Voxel voxel() const {
		return _voxel;
	}

	bool operator()(const WorldData::Sampler& sampler);
};

}
