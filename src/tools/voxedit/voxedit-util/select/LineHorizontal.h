/**
 * @file
 */

#pragma once

#include "Select.h"

namespace voxedit {
namespace selections {

/**
 * @brief Selects all voxel along the line horizontally
 */
class LineHorizontal : public Select {
public:
	SelectionSingleton(LineHorizontal)

	int execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) override;
};

}
}
