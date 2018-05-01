/**
 * @file
 */

#include "Same.h"

namespace voxedit {
namespace selections {

int Same::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) {
	selection.setVoxel(model.voxel());
	const glm::ivec3& pos = model.position();
	int cnt = 0;
	goSixDirections(model, selection, model.voxel(), cnt);
	selection.setPosition(pos);
	model.setPosition(pos);
	return cnt;
}

}
}
