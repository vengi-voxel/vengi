#include "Fill.h"

namespace voxedit {
namespace tool {

void fill(voxel::RawVolume& target, const glm::ivec3& position, const Axis axis, const voxel::Voxel& voxel, bool overwrite) {
	const voxel::Region& region = target.getRegion();
	int zStart = region.getLowerZ();
	if ((axis & Axis::Z) != Axis::None) {
		zStart = position.y;
	}
	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		int yStart = region.getLowerY();
		if ((axis & Axis::Y) != Axis::None) {
			yStart = position.y;
		}
		for (int32_t y = yStart; y <= region.getUpperY(); ++y) {
			int xStart = region.getLowerX();
			if ((axis & Axis::X) != Axis::None) {
				xStart = position.x;
			}
			for (int32_t x = xStart; x <= region.getUpperX(); ++x) {
				if (overwrite || isAir(target.getVoxel(x, y, z).getMaterial())) {
					target.setVoxel(x, y, z, voxel);
				}
				if ((axis & Axis::X) != Axis::None) {
					break;
				}
			}
			if ((axis & Axis::Y) != Axis::None) {
				break;
			}
		}
		if ((axis & Axis::Z) != Axis::None) {
			break;
		}
	}
}

}
}
