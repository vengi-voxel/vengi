/**
 * @file
 */

#pragma once

#include "Select.h"

namespace voxedit {
namespace selections {

class Edge : public Select {
public:
	SelectionSingleton(Edge)

	int execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) override;
};

}
}
