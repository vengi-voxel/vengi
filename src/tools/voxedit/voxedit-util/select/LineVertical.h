/**
 * @file
 */

#pragma once

#include "Select.h"

namespace voxedit {
namespace selections {

/**
 * @brief Selects all voxel along the line vertically
 */
class LineVertical : public Select {
public:
	SelectionSingleton(LineVertical)

	int execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) override;
};

}
}
