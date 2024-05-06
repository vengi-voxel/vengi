/**
 * @file
 */

#include "Clipboard.h"
#include "core/Log.h"
#include "voxedit-util/modifier/Selection.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxelutil/VolumeMerger.h"
#include <glm/common.hpp>

namespace voxedit {
namespace tool {

voxel::VoxelData copy(const voxel::VoxelData &voxelData, const Selections &selections) {
	if (!voxelData) {
		Log::debug("Copy failed: no voxel data");
		return {};
	}
	if (selections.empty()) {
		Log::debug("Copy failed: no selection active");
		return {};
	}

	voxel::RawVolume *v = new voxel::RawVolume(voxelData.volume, selections);
	return voxel::VoxelData(v, voxelData.palette, true);
}

voxel::VoxelData cut(voxel::VoxelData &voxelData, const Selections &selections, voxel::Region &modifiedRegion) {
	if (!voxelData) {
		Log::debug("Copy failed: no voxel data");
		return {};
	}

	if (selections.empty()) {
		Log::debug("Cut failed: no selection active");
		return {};
	}

	voxel::RawVolume *v = new voxel::RawVolume(voxelData.volume, selections);
	for (const Selection &selection : selections) {
		const glm::ivec3 &mins = selection.getLowerCorner();
		const glm::ivec3 &maxs = selection.getUpperCorner();
		static constexpr voxel::Voxel AIR;
		voxel::RawVolumeWrapper wrapper(voxelData.volume, selection);
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
	return {v, voxelData.palette, true};
}

void paste(voxel::VoxelData &out, const voxel::VoxelData &in, const glm::ivec3 &referencePosition,
		   voxel::Region &modifiedRegion) {
	if (!in) {
		Log::debug("Paste failed: no in voxel data");
		return;
	}
	if (!out) {
		Log::debug("Paste failed: no out voxel data");
		return;
	}

	voxel::Region region = in.volume->region();
	region.shift(-region.getLowerCorner());
	region.shift(referencePosition);
	modifiedRegion = region;
	voxelutil::mergeVolumes(out.volume, *out.palette, in.volume, *in.palette, region, in.volume->region(),
							voxelutil::MergeSkipEmpty());
	Log::debug("Pasted %s", modifiedRegion.toString().c_str());
}

} // namespace tool
} // namespace voxedit
