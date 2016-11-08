#pragma once

#include "Select.h"

namespace selections {

class LineVertical : public Select {
public:
	SelectionSingleton(LineVertical)

	bool execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const override;
};

}
