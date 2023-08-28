/**
 * @file
 */

#include "Clipboard.h"
#include "voxedit-util/modifier/Selection.h"
#include "voxelutil/VolumeMerger.h"
#include "voxelutil/VolumeCropper.h"
#include "voxel/RawVolumeWrapper.h"
#include "core/Log.h"
#include <glm/common.hpp>

namespace voxedit {
namespace tool {

voxel::RawVolume* copy(const voxel::RawVolume *volume, const Selections &selections) {
	if (volume == nullptr) {
		return nullptr;
	}
	if (selections.empty()) {
		Log::debug("Copy failed: no selection active");
		return nullptr;
	}

	return new voxel::RawVolume(volume, selections);
}

voxel::RawVolume* cut(voxel::RawVolume *volume, const Selections &selections, voxel::Region& modifiedRegion) {
	if (selections.empty()) {
		Log::debug("Cut failed: no selection active");
		return nullptr;
	}

	voxel::RawVolume* v = new voxel::RawVolume(volume, selections);
	for (const Selection &selection : selections) {
		const glm::ivec3& mins = selection.getLowerCorner();
		const glm::ivec3& maxs = selection.getUpperCorner();
		static constexpr voxel::Voxel AIR;
		voxel::RawVolumeWrapper wrapper(volume, selection);
		for (int32_t x = mins.x; x <= maxs.x; ++x) {
			for (int32_t y = mins.y; y <= maxs.y; ++y) {
				for (int32_t z = mins.z; z <= maxs.z; ++z) {
					wrapper.setVoxel(x, y, z, AIR);
				}
			}
		}
		if (wrapper.dirtyRegion().isValid()) {
			if (modifiedRegion.isValid()) {
				modifiedRegion.accumulate(wrapper.dirtyRegion());
			} else {
				modifiedRegion = wrapper.dirtyRegion();
			}
		}
	}
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
