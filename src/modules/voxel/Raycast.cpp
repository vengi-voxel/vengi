#include "Raycast.h"

namespace voxel {

bool Raycast::operator()(const WorldData::Sampler& sampler) {
	if (sampler.getVoxel().getMaterial() != Air) {
		_voxel = sampler.getVoxel();
		_position = glm::vec3(sampler.getPosition().getX(), sampler.getPosition().getY(), sampler.getPosition().getZ());
		return false;
	}

	return true;
}

}
