#include "Same.h"

namespace voxedit {
namespace selections {

int Same::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	selection.setVoxel(model.getVoxel());
	const glm::ivec3& pos = model.getPosition();
	int cnt = 0;
	goSixDirections(model, selection, model.getVoxel(), cnt);
	selection.setPosition(pos);
	model.setPosition(pos);
	return cnt;
}

}
}
