#pragma once

#include "Select.h"

namespace selections {

class Same : public Select {
public:
	SelectionSingleton(Same)

	bool execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const override;
};

}
