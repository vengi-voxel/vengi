/**
 * @file
 */

#include "VolumeSplitter.h"
#include "app/Async.h"
#include "core/Log.h"
#include "voxel/Connectivity.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

namespace priv {

// special voxel to mark already visited voxels
const uint8_t visitedFlag = 1u;
const voxel::Voxel visited(voxel::VoxelType::Air, 1, visitedFlag);

} // namespace priv

static void processNeighbours(voxel::RawVolume &volume, voxel::RawVolume &object, const glm::ivec3 &position, voxel::Connectivity connectivity) {
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

	switch (connectivity) {
	case voxel::Connectivity::TwentySixConnected:
		for (int i = 0; i < lengthof(voxel::arrayPathfinderCorners); ++i) {
			const glm::ivec3 p = position + voxel::arrayPathfinderCorners[i];
			processNeighbours(volume, object, p, connectivity);
		}
		/* fallthrough */

	case voxel::Connectivity::EighteenConnected:
		for (int i = 0; i < lengthof(voxel::arrayPathfinderEdges); ++i) {
			const glm::ivec3 p = position + voxel::arrayPathfinderEdges[i];
			processNeighbours(volume, object, p, connectivity);
		}
		/* fallthrough */

	case voxel::Connectivity::SixConnected:
		for (int i = 0; i < lengthof(voxel::arrayPathfinderFaces); ++i) {
			const glm::ivec3 p = position + voxel::arrayPathfinderFaces[i];
			processNeighbours(volume, object, p, connectivity);
		}
		break;
	}
}

core::Buffer<voxel::RawVolume *> splitObjects(const voxel::RawVolume *volume, VisitorOrder order,
											  voxel::Connectivity connectivity) {
	voxel::RawVolume copy(*volume);
	copy.setBorderValue(priv::visited);

	core::Buffer<voxel::RawVolume *> rawVolumes;

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
		processNeighbours(copy, object, position, connectivity);
		if (voxel::RawVolume *v = voxelutil::cropVolume(&object)) {
			rawVolumes.push_back(v);
		} else {
			rawVolumes.push_back(new voxel::RawVolume(object));
		}
	}, VisitAll(), order);

	return rawVolumes;
}

core::Buffer<voxel::RawVolume *> splitVolume(const voxel::RawVolume *volume, const glm::ivec3 &maxSize,
											 bool createEmpty) {
	const voxel::Region &region = volume->region();
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();

	core::Buffer<voxel::RawVolume *> rawVolumes;

	const glm::ivec3 step = glm::min(region.getDimensionsInVoxels(), maxSize);
	Log::debug("split region: %s", region.toString().c_str());
	const glm::ivec3 steps = (region.getDimensionsInVoxels() + (step - 1)) / step;
	rawVolumes.resize(steps.x * steps.y * steps.z);
	auto fn = [step, maxs, volume, maxSize, mins, createEmpty, steps, &rawVolumes](int start, int end) {
		for (int i = start; i < end; ++i) {
			int idx = i * steps.x * steps.z;
			const int y = i * step.y;
			for (int z = mins.z; z <= maxs.z; z += step.z) {
				for (int x = mins.x; x <= maxs.x; x += step.x) {
					const glm::ivec3 innerMins(x, y, z);
					const glm::ivec3 innerMaxs = glm::min(maxs, innerMins + maxSize - 1);
					const voxel::Region innerRegion(innerMins, innerMaxs);
					bool onlyAir = true;
					voxel::RawVolume *copy = new voxel::RawVolume(*volume, innerRegion, createEmpty ? nullptr : &onlyAir);
					if (onlyAir && !createEmpty) {
						Log::debug("- skip empty %s", innerRegion.toString().c_str());
						delete copy;
						++idx;
						continue;
					}
					core_assert((int)rawVolumes.capacity() > idx);
					rawVolumes[idx++] = copy;
				}
			}
		}
	};
	app::for_parallel(0, steps.y, fn);
	return rawVolumes;
}

} // namespace voxelutil
