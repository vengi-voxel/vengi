#include "ShapeGenerator.h"
#include "voxel/polyvox/PagedVolume.h"
#include "voxel/polyvox/RawVolume.h"

namespace voxel {
namespace shape {

// http://members.chello.at/~easyfilter/bresenham.html
void createLine(PagedVolume& volume, const glm::ivec3& start, const glm::ivec3& end, const Voxel& voxel) {
	voxel::raycastWithEndpoints(&volume, start, end, [&] (auto& sampler) {
		sampler.setVoxel(voxel);
		return true;
	});
}

void createLine(RawVolume& volume, const glm::ivec3& start, const glm::ivec3& end, const Voxel& voxel) {
	voxel::raycastWithEndpoints(&volume, start, end, [&] (auto& sampler) {
		sampler.setVoxel(voxel);
		return true;
	});
}

}
}
