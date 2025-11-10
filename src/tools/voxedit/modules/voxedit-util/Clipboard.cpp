/**
 * @file
 */

#include "Clipboard.h"
#include "core/Log.h"
#include "voxedit-util/modifier/SelectionManager.h"
#include "voxelutil/VolumeMerger.h"
#include <glm/common.hpp>

namespace voxedit {
namespace tool {

voxel::VoxelData copy(const voxel::VoxelData &voxelData, const SelectionManagerPtr &selectionMgr) {
	if (!voxelData) {
		Log::debug("Copy failed: no voxel data");
		return {};
	}
	if (!selectionMgr->hasSelection()) {
		Log::debug("Copy failed: no selection active");
		return {};
	}

	voxel::RawVolume *v = new voxel::RawVolume(voxelData.volume, selectionMgr->region());
	return voxel::VoxelData(v, voxelData.palette, true);
}

voxel::VoxelData cut(voxel::VoxelData &voxelData, const SelectionManagerPtr &selectionMgr, voxel::Region &modifiedRegion) {
	if (!voxelData) {
		Log::debug("Copy failed: no voxel data");
		return {};
	}

	if (!selectionMgr->hasSelection()) {
		Log::debug("Cut failed: no selection active");
		return {};
	}

	voxel::RawVolume *v = selectionMgr->cut(*voxelData.volume);
	if (modifiedRegion.isValid()) {
		modifiedRegion.accumulate(v->region());
	} else {
		modifiedRegion = v->region();
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
	voxelutil::mergeVolumes(out.volume, *out.palette, in.volume, *in.palette, region, in.volume->region());
	Log::debug("Pasted %s", modifiedRegion.toString().c_str());
}

} // namespace tool
} // namespace voxedit
