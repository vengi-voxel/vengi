/**
 * @file
 */

#pragma once

#include "Select.h"

namespace voxedit {
namespace selections {

/**
 * @brief Selects all voxels that are touching the selected voxel and are of the same type.
 */
class Same : public Select {
public:
	SelectionSingleton(Same)

	int execute(voxel::RawVolume::Sampler& model, voxel::RawVolume::Sampler& selection) override;
};

}
}
