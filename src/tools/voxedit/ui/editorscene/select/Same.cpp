#include "Same.h"

namespace voxedit {
namespace selections {

bool Same::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	selection.setVoxel(model.getVoxel());
	const glm::ivec3& pos = model.getPosition();
	goSixDirections(model, selection, model.getVoxel());
	selection.setPosition(pos);
	model.setPosition(pos);
	return true;
}

}
}
