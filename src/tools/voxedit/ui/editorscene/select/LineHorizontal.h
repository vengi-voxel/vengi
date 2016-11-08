#pragma once

#include "Select.h"

namespace selections {

class LineHorizontal : public Select {
public:
	SelectionSingleton(LineHorizontal)

	bool execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) const override;
};

}
