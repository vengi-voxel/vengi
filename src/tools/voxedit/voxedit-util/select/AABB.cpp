/**
 * @file
 */

#include "AABB.h"
#include "core/Log.h"

namespace voxedit {
namespace selections {

void AABB::unselect() {
	_started = false;
	Log::debug("Unselect all for AABB");
}

int AABB::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) {
	if (!_started) {
		_mins = model.position();
		_maxs = _mins;
		_started = true;
		Log::debug("Start to select in aabb mode");
	} else {
		const glm::ivec3& pos = model.position();
		glm::ivec3 mins;
		glm::ivec3 maxs;
		mins.x = glm::min(_mins.x, pos.x);
		mins.y = glm::min(_mins.y, pos.y);
		mins.z = glm::min(_mins.z, pos.z);
		maxs.x = glm::max(_mins.x, pos.x);
		maxs.y = glm::max(_mins.y, pos.y);
		maxs.z = glm::max(_mins.z, pos.z);

		if (glm::any(glm::greaterThan(mins, _mins))) {
			Log::debug("unselect mins - (%i:%i:%i) - (%i:%i:%i)", _mins.x, _mins.y, _mins.z, mins.x, mins.y, mins.z);
			for (int x = _mins.x; x < mins.x; ++x) {
				for (int y = _mins.y; y < mins.y; ++y) {
					for (int z = _mins.z; z < mins.z; ++z) {
						glm::ivec3 pos(x, y, z);
						selection.setPosition(pos);
						selection.setVoxel(voxel::createVoxel(voxel::VoxelType::Air, 0));
					}
				}
			}
		}
		if (glm::any(glm::lessThan(maxs, _maxs))) {
			Log::debug("unselect maxs - (%i:%i:%i) - (%i:%i:%i)", maxs.x, maxs.y, maxs.z, _maxs.x, _maxs.y, _maxs.z);
			for (int x = maxs.x; x < _maxs.x; ++x) {
				for (int y = maxs.y; y < _maxs.y; ++y) {
					for (int z = maxs.z; z < _maxs.z; ++z) {
						glm::ivec3 pos(x, y, z);
						selection.setPosition(pos);
						selection.setVoxel(voxel::createVoxel(voxel::VoxelType::Air, 0));
					}
				}
			}
		}
		_mins = mins;
		_maxs = maxs;
	}

	Log::debug("Select from (%i:%i:%i) to (%i:%i:%i)", _mins.x, _mins.y, _mins.z, _maxs.x, _maxs.y, _maxs.z);

	int cnt = 0;
	const glm::ivec3& pos = model.position();
	for (int x = _mins.x; x <= _maxs.x; ++x) {
		for (int y = _mins.y; y <= _maxs.y; ++y) {
			for (int z = _mins.z; z <= _maxs.z; ++z) {
				glm::ivec3 pos(x, y, z);
				selection.setPosition(pos);
				model.setPosition(pos);
				if (selection.setVoxel(model.voxel())) {
					++cnt;
				}
			}
		}
	}
	selection.setPosition(pos);
	model.setPosition(pos);
	return cnt;
}

}
}
