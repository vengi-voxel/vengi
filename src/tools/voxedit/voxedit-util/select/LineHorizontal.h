/**
 * @file
 */

#pragma once

#include "Select.h"

namespace voxedit {
namespace selections {

class LineHorizontal : public Select {
public:
	SelectionSingleton(LineHorizontal)

	int execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) override;
};

}
}
