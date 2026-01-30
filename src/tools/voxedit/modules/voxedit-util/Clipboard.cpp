/**
 * @file
 */

#include "Clipboard.h"
#include "core/Log.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeMerger.h"
#include <glm/common.hpp>

namespace voxedit {
namespace tool {

voxel::ClipboardData copy(const scenegraph::SceneGraphNode &node) {
	if (!node.isModelNode()) {
		Log::debug("Copy failed: not a model node");
		return {};
	}
	const voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		Log::debug("Copy failed: no voxel data");
		return {};
	}
	if (!node.hasSelection()) {
		Log::debug("Copy failed: no selection active");
		return {};
	}

	// Calculate the bounding region of selected voxels
	voxel::Region selectionRegion = voxel::Region::InvalidRegion;
	const voxel::Region &region = volume->region();
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();

	for (int32_t z = mins.z; z <= maxs.z; ++z) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
			for (int32_t x = mins.x; x <= maxs.x; ++x) {
				const voxel::Voxel &voxel = volume->voxel(x, y, z);
				if ((voxel.getFlags() & voxel::FlagOutline) != 0) {
					if (selectionRegion.isValid()) {
						selectionRegion.accumulate(x, y, z);
					} else {
						selectionRegion = voxel::Region(x, y, z, x, y, z);
					}
				}
			}
		}
	}

	if (!selectionRegion.isValid()) {
		Log::debug("Copy failed: no selected voxels found");
		return {};
	}

	// Create a new volume with the selected voxels
	voxel::RawVolume *v = new voxel::RawVolume(selectionRegion);
	const glm::ivec3 &selMins = selectionRegion.getLowerCorner();
	const glm::ivec3 &selMaxs = selectionRegion.getUpperCorner();

	for (int32_t z = selMins.z; z <= selMaxs.z; ++z) {
		for (int32_t y = selMins.y; y <= selMaxs.y; ++y) {
			for (int32_t x = selMins.x; x <= selMaxs.x; ++x) {
				const voxel::Voxel &voxel = volume->voxel(x, y, z);
				if ((voxel.getFlags() & voxel::FlagOutline) != 0) {
					// Copy voxel without the outline flag
					voxel::Voxel copiedVoxel = voxel::createVoxel(voxel.getMaterial(), voxel.getColor(),
																  voxel.getNormal(), 0, voxel.getBoneIdx());
					v->setVoxel(x, y, z, copiedVoxel);
				}
			}
		}
	}
	return voxel::ClipboardData(v, node.palette(), true);
}

voxel::ClipboardData cut(scenegraph::SceneGraphNode &node, voxel::Region &modifiedRegion) {
	if (!node.isModelNode()) {
		Log::debug("Cut failed: not a model node");
		return {};
	}
	voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		Log::debug("Cut failed: no voxel data");
		return {};
	}
	if (!node.hasSelection()) {
		Log::debug("Cut failed: no selection active");
		return {};
	}

	// Calculate the bounding region of selected voxels
	voxel::Region selectionRegion = voxel::Region::InvalidRegion;
	const voxel::Region &region = volume->region();
	const glm::ivec3 &mins = region.getLowerCorner();
	const glm::ivec3 &maxs = region.getUpperCorner();

	for (int32_t z = mins.z; z <= maxs.z; ++z) {
		for (int32_t y = mins.y; y <= maxs.y; ++y) {
			for (int32_t x = mins.x; x <= maxs.x; ++x) {
				const voxel::Voxel &voxel = volume->voxel(x, y, z);
				if ((voxel.getFlags() & voxel::FlagOutline) != 0) {
					if (selectionRegion.isValid()) {
						selectionRegion.accumulate(x, y, z);
					} else {
						selectionRegion = voxel::Region(x, y, z, x, y, z);
					}
				}
			}
		}
	}

	if (!selectionRegion.isValid()) {
		Log::debug("Cut failed: no selected voxels found");
		return {};
	}

	// Create a new volume with the selected voxels
	voxel::RawVolume *v = new voxel::RawVolume(selectionRegion);
	const glm::ivec3 &selMins = selectionRegion.getLowerCorner();
	const glm::ivec3 &selMaxs = selectionRegion.getUpperCorner();

	for (int32_t z = selMins.z; z <= selMaxs.z; ++z) {
		for (int32_t y = selMins.y; y <= selMaxs.y; ++y) {
			for (int32_t x = selMins.x; x <= selMaxs.x; ++x) {
				voxel::Voxel voxel = volume->voxel(x, y, z);
				if ((voxel.getFlags() & voxel::FlagOutline) != 0) {
					// Copy voxel without the outline flag
					voxel::Voxel copiedVoxel = voxel::createVoxel(voxel.getMaterial(), voxel.getColor(),
																  voxel.getNormal(), 0, voxel.getBoneIdx());
					v->setVoxel(x, y, z, copiedVoxel);
					// Clear the original voxel
					volume->setVoxel(x, y, z, voxel::Voxel());
				}
			}
		}
	}

	if (modifiedRegion.isValid()) {
		modifiedRegion.accumulate(v->region());
	} else {
		modifiedRegion = v->region();
	}
	return {v, node.palette(), true};
}

void paste(voxel::ClipboardData &out, const voxel::ClipboardData &in, const glm::ivec3 &referencePosition,
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
