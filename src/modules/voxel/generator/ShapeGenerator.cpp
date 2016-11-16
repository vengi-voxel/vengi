#include "ShapeGenerator.h"
#include "voxel/polyvox/PagedVolume.h"
#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"
#include "voxel/polyvox/RawVolumeWrapper.h"

namespace voxel {
namespace shape {

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

void createLine(PagedVolumeWrapper& volume, const glm::ivec3& start, const glm::ivec3& end, const Voxel& voxel) {
	createLine(*volume.getVolume(), start, end, voxel);
}

void createLine(RawVolumeWrapper& volume, const glm::ivec3& start, const glm::ivec3& end, const Voxel& voxel) {
	createLine(*volume.getVolume(), start, end, voxel);
}

}
}
