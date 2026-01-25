/**
 * @file
 */

#include "Clipboard.h"
#include "core/Log.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/modifier/SelectionManager.h"
#include "voxelutil/VolumeMerger.h"
#include <glm/common.hpp>

namespace voxedit {
namespace tool {

voxel::ClipboardData copy(const scenegraph::SceneGraphNode &node, const SelectionManagerPtr &selectionMgr) {
	if (!node.isModelNode()) {
		Log::debug("Copy failed: not a model node");
		return {};
	}
	const voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		Log::debug("Copy failed: no voxel data");
		return {};
	}
	voxel::RawVolume *v = selectionMgr->copy(node);
	if (v == nullptr) {
		Log::debug("Copy failed: no selection active");
		return {};
	}
	return voxel::ClipboardData(v, node.palette(), true);
}

voxel::ClipboardData cut(scenegraph::SceneGraphNode &node, const SelectionManagerPtr &selectionMgr, voxel::Region &modifiedRegion) {
	if (!node.isModelNode()) {
		Log::debug("Cut failed: not a model node");
		return {};
	}
	voxel::RawVolume *volume = node.volume();
	if (volume == nullptr) {
		Log::debug("Cut failed: no voxel data");
		return {};
	}

	voxel::RawVolume *v = selectionMgr->cut(node);
	if (!v) {
		Log::debug("Cut failed: no selection active");
		return {};
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
