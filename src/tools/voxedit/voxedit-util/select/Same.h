/**
 * @file
 */

#pragma once

#include "Select.h"

namespace voxedit {
namespace selections {

class Same : public Select {
public:
	SelectionSingleton(Same)

	int execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) override;
};

}
}
