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
#include "math/Vec3i.h"

namespace voxelutil {

namespace priv {

// special voxel to mark already visited voxels
const uint8_t visitedFlag = 1u;
const voxel::Voxel visited(voxel::VoxelType::Air, 1, visitedFlag);

} // namespace priv

static void processNeighbours(voxel::RawVolume &volume, voxel::RawVolume &object, const math::Vec3i &position, voxel::Connectivity connectivity) {
	if (!volume.region().containsPoint(position)) {
		return;
	}
	voxel::Voxel voxel = volume.voxel(position);
	if (voxel.getFlags() == priv::visitedFlag) {
		return;
	}
	if (voxel::isAir(voxel.getMaterial())) {
		volume.setVoxel(position, priv::visited);
		return;
	}

	object.setVoxel(position, voxel);
	volume.setVoxel(position, priv::visited);

	switch (connectivity) {
	case voxel::Connectivity::TwentySixConnected:
		for (int i = 0; i < lengthof(voxel::arrayPathfinderCorners); ++i) {
			const math::Vec3i p = position + voxel::arrayPathfinderCorners[i];
			processNeighbours(volume, object, p, connectivity);
		}
		/* fallthrough */

	case voxel::Connectivity::EighteenConnected:
		for (int i = 0; i < lengthof(voxel::arrayPathfinderEdges); ++i) {
			const math::Vec3i p = position + voxel::arrayPathfinderEdges[i];
			processNeighbours(volume, object, p, connectivity);
		}
		/* fallthrough */

	case voxel::Connectivity::SixConnected:
		for (int i = 0; i < lengthof(voxel::arrayPathfinderFaces); ++i) {
			const math::Vec3i p = position + voxel::arrayPathfinderFaces[i];
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
		const math::Vec3i position(x, y, z);
		if (voxel::isAir(voxel.getMaterial())) {
			copy.setVoxel(position, priv::visited);
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

core::Buffer<voxel::RawVolume *> splitVolume(const voxel::RawVolume *volume, const math::Vec3i &maxSize,
											 bool createEmpty) {
	const voxel::Region &region = volume->region();
	const math::Vec3i &mins = region.getLowerCorner();
	const math::Vec3i &maxs = region.getUpperCorner();

	core::Buffer<voxel::RawVolume *> rawVolumes;

	const math::Vec3i clampedSize = math::max(maxSize, math::Vec3i(1));
	const math::Vec3i step = math::min(region.getDimensionsInVoxels(), clampedSize);
	Log::debug("split region: %s", region.toString().c_str());
	const math::Vec3i steps = (region.getDimensionsInVoxels() + (step - 1)) / step;
	rawVolumes.resize(steps.x * steps.y * steps.z);
	auto fn = [step, maxs, volume, clampedSize, mins, createEmpty, steps, &rawVolumes](int start, int end) {
		for (int i = start; i < end; ++i) {
			int idx = i * steps.x * steps.z;
			const int y = mins.y + i * step.y;
			for (int z = mins.z; z <= maxs.z; z += step.z) {
				for (int x = mins.x; x <= maxs.x; x += step.x) {
					const math::Vec3i innerMins(x, y, z);
					const math::Vec3i innerMaxs = math::min(maxs, innerMins + clampedSize - 1);
					const voxel::Region innerRegion(innerMins, innerMaxs);
					bool onlyAir = true;
					voxel::RawVolume *copy = new voxel::RawVolume(*volume, innerRegion, createEmpty ? nullptr : &onlyAir);
					if (!copy->valid()) {
						Log::error("Failed to allocate split volume for region %s", innerRegion.toString().c_str());
						delete copy;
						++idx;
						continue;
					}
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
