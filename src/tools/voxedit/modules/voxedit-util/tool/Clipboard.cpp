/**
 * @file
 */

#include "Clipboard.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeCropper.h"
#include "voxel/RawVolumeWrapper.h"
#include "core/Log.h"
#include <glm/common.hpp>

namespace voxedit {
namespace tool {

voxel::RawVolume* copy(const voxel::RawVolume *volume, const Selection &selection) {
	if (!selection.isValid()) {
		Log::debug("Copy failed: Source region is invalid: %s", selection.toString().c_str());
		return nullptr;
	}
	return new voxel::RawVolume(volume, selection);
}

voxel::RawVolume* cut(voxel::RawVolume *volume, const Selection &selection, voxel::Region& modifiedRegion) {
	if (!selection.isValid()) {
		Log::debug("Cut failed: Source region is invalid: %s", selection.toString().c_str());
		return nullptr;
	}

	voxel::RawVolume* v = new voxel::RawVolume(volume, selection);
	const glm::ivec3& mins = v->region().getLowerCorner();
	const glm::ivec3& maxs = v->region().getUpperCorner();
	static constexpr voxel::Voxel AIR;
	voxel::RawVolumeWrapper wrapper(volume, v->region());
	for (int32_t x = mins.x; x <= maxs.x; ++x) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
			for (int32_t z = mins.z; z <= maxs.z; ++z) {
				wrapper.setVoxel(x, y, z, AIR);
			}
		}
	}
	modifiedRegion = wrapper.dirtyRegion();
	return v;
}

void paste(voxel::RawVolume* out, const voxel::RawVolume* in, const glm::ivec3& referencePosition, voxel::Region& modifiedRegion) {
	voxel::Region destReg = out->region();
	destReg.shift(referencePosition);
	voxel::Region sourceReg = in->region();

	if (!destReg.isValid()) {
		Log::debug("Paste failed: Destination region is invalid: %s", destReg.toString().c_str());
		return;
	}

	voxel::RawVolume::Sampler srcSampler(in);
	voxel::RawVolume::Sampler destSampler(out);
	const glm::ivec3& srcMins = sourceReg.getLowerCorner();
	const glm::ivec3& srcMaxs = sourceReg.getUpperCorner();
	const glm::ivec3& destMins = destReg.getLowerCorner();

	glm::ivec3 modifiedMins((std::numeric_limits<int>::max)() / 2);
	glm::ivec3 modifiedMaxs((std::numeric_limits<int>::min)() / 2);

	for (int32_t x = srcMins.x, destX = destMins.x; x <= srcMaxs.x; ++x, ++destX) {
		for (int32_t y = sourceReg.getLowerY(), destY = destMins.y; y <= sourceReg.getUpperY(); ++y, ++destY) {
			srcSampler.setPosition(x, y, srcMins.z);
			if (!destSampler.setPosition(destX, destY, destMins.z)) {
				continue;
			}
			for (int32_t z = srcMins.z; z <= srcMaxs.z; ++z) {
				const voxel::Voxel& voxel = srcSampler.voxel();
				if (destSampler.setVoxel(voxel)) {
					modifiedMins = (glm::min)(modifiedMins, destSampler.position());
					modifiedMaxs = (glm::max)(modifiedMaxs, destSampler.position());
				}
				srcSampler.movePositiveZ();
				destSampler.movePositiveZ();
				if (!destSampler.currentPositionValid()) {
					break;
				}
			}
		}
	}

	modifiedRegion = voxel::Region(modifiedMins, modifiedMaxs);
	Log::debug("Pasted %s", modifiedRegion.toString().c_str());
}

}
}
