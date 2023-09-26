/**
 * @file
 */

#include "VolumeSplitter.h"
#include "core/Common.h"
#include "core/Log.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"

namespace voxelutil {

namespace priv {

const glm::ivec3 neighbours[6] = {glm::ivec3(0, 0, -1), glm::ivec3(0, 0, +1), glm::ivec3(0, -1, 0),
								  glm::ivec3(0, +1, 0), glm::ivec3(-1, 0, 0), glm::ivec3(+1, 0, 0)};

// special voxel to mark already visited voxels
const uint8_t visitedFlag = 1u;
const voxel::Voxel visited(voxel::VoxelType::Air, 1, visitedFlag);

} // namespace priv

static void processNeighbours(voxel::RawVolume &volume, voxel::RawVolume &object, const glm::ivec3 position) {
	if (!volume.region().containsPoint(position)) {
		return;
	}
	voxel::Voxel voxel = volume.voxel(position);
	if (voxel.getFlags() == priv::visitedFlag) {
		return;
	}
	if (voxel::isAir(voxel.getMaterial())) {
		volume.setVoxelUnsafe(position, priv::visited);
		return;
	}

	object.setVoxel(position, voxel);
	volume.setVoxelUnsafe(position, priv::visited);

	for (int i = 0; i < 6; ++i) {
		const glm::ivec3 p = position + priv::neighbours[i];
		processNeighbours(volume, object, p);
	}
}

void splitObjects(const voxel::RawVolume *v, core::DynamicArray<voxel::RawVolume *> &rawVolumes, VisitorOrder order) {
	voxel::RawVolume copy(*v);
	copy.setBorderValue(priv::visited);

	visitVolume(copy, [&](int x, int y, int z, const voxel::Voxel &voxel) {
		if (voxel.getFlags() == priv::visitedFlag) {
			return;
		}
		const glm::ivec3 position(x, y, z);
		if (voxel::isAir(voxel.getMaterial())) {
			copy.setVoxelUnsafe(position, priv::visited);
			return;
		}

		voxel::RawVolume object(copy.region());
		processNeighbours(copy, object, position);
		rawVolumes.push_back(voxelutil::cropVolume(&object));
	}, VisitAll(), order);
}

void splitVolume(const voxel::RawVolume *volume, const glm::ivec3 &maxSize,
				 core::DynamicArray<voxel::RawVolume *> &rawVolumes, bool createEmpty) {
	const voxel::Region &region = volume->region();
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();

	const glm::ivec3 step = glm::min(region.getDimensionsInVoxels(), maxSize);
	Log::debug("split region: %s", region.toString().c_str());
	for (int y = mins.y; y <= maxs.y; y += step.y) {
		for (int z = mins.z; z <= maxs.z; z += step.z) {
			for (int x = mins.x; x <= maxs.x; x += step.x) {
				const glm::ivec3 innerMins(x, y, z);
				const glm::ivec3 innerMaxs = glm::min(maxs, innerMins + maxSize - 1);
				const voxel::Region innerRegion(innerMins, innerMaxs);
				voxel::RawVolume *copy = new voxel::RawVolume(innerRegion);
				if (voxelutil::copy(*volume, innerRegion, *copy, innerRegion)) {
					Log::debug("- split %s", innerRegion.toString().c_str());
				} else if (!createEmpty) {
					Log::debug("- skip empty %s", innerRegion.toString().c_str());
					delete copy;
					continue;
				}
				rawVolumes.push_back(copy);
			}
		}
	}
}

} // namespace voxelutil
