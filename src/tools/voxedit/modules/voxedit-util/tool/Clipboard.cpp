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
	voxel::Region region = in->region();
	region.shift(-region.getLowerCorner());
	region.shift(referencePosition);
	modifiedRegion = region;
	voxelutil::mergeVolumes(out, in, region, in->region(), voxelutil::MergeSkipEmpty());
	Log::debug("Pasted %s", modifiedRegion.toString().c_str());
}

}
}
