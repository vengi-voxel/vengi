#include "LineHorizontal.h"

namespace selections {

bool LineHorizontal::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	selection.setVoxel(model.getVoxel());
	const glm::ivec3& pos = model.getPosition();
	goLeft(model, selection);

	selection.setPosition(pos);
	model.setPosition(pos);
	goRight(model, selection);

	selection.setPosition(pos);
	model.setPosition(pos);
	return true;
}

}
