#pragma once

#include "Select.h"

namespace selections {

class Single : public Select {
public:
	SelectionSingleton(Single)

	bool execute(const voxel::RawVolume *model, voxel::RawVolume *selection, const glm::ivec3& pos) const override {
		if (!model->getEnclosingRegion().containsPoint(pos)) {
			return false;
		}
		const voxel::Voxel& voxel = model->getVoxel(pos);
		selection->setVoxel(pos, voxel);
		return false;
	}
};

}
