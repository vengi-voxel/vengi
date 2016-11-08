#include "Same.h"

namespace selections {

bool Same::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	const glm::ivec3& pos = model.getPosition();
	goSixDirections(model, selection, model.getVoxel());

	selection.setPosition(pos);
	model.setPosition(pos);
	goRight(model, selection);

	selection.setPosition(pos);
	model.setPosition(pos);
	return true;
}

}
