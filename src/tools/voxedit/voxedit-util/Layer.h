/**
 * @file
 */

#include "voxelrender/RawVolumeRenderer.h"
#include <string>
#include <array>

namespace voxedit {

struct Layer {
	std::string name;
	bool visible = true;
	bool valid = false;

	void reset() {
		name.clear();
		visible = true;
		valid = false;
	}
};
using Layers = std::array<Layer, voxelrender::RawVolumeRenderer::MAX_VOLUMES>;

}

