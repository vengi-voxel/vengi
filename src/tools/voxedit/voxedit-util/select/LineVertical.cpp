/**
 * @file
 */

#include "LineVertical.h"

namespace voxedit {
namespace selections {

int LineVertical::execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) {
	int cnt = 0;
	if (selection.setVoxel(model.voxel())) {
		++cnt;
	}
	const glm::ivec3& pos = model.position();

	goUp(model, selection, cnt);
	selection.setPosition(pos);
	model.setPosition(pos);

	goDown(model, selection, cnt);
	selection.setPosition(pos);
	model.setPosition(pos);
	return cnt;
}

}
}
