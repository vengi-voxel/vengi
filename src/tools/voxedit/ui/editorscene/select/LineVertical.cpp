#include "LineVertical.h"

namespace voxedit {
namespace selections {

bool LineVertical::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const {
	selection.setVoxel(model.getVoxel());
	const glm::ivec3& pos = model.getPosition();

	goUp(model, selection);
	selection.setPosition(pos);
	model.setPosition(pos);

	goDown(model, selection);
	selection.setPosition(pos);
	model.setPosition(pos);
	return true;
}

}
}
