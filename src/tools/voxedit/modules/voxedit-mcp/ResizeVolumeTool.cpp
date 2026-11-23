/**
 * @file
 */

#include "ResizeVolumeTool.h"
#include "Authoring.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxedit-util/SceneManager.h"
#include "math/Vec3i.h"

namespace voxedit {

static constexpr int MaxAxisVoxels = 512;

static bool axisTooLarge(const voxel::Region &region) {
	const math::Vec3i &dim = region.getDimensionsInVoxels();
	return dim.x > MaxAxisVoxels || dim.y > MaxAxisVoxels || dim.z > MaxAxisVoxels;
}

ResizeVolumeTool::ResizeVolumeTool() : Tool("voxedit_resize_volume") {
	_tool.set("description",
			  "Resize a model node's volume. size is [x, y, z] voxel counts and keeps the current lower corner. "
			  "region is inclusive {minX, minY, minZ, maxX, maxY, maxZ}. "
			  "fit shrinks or grows the volume around the existing voxels; padding adds empty cells around them. "
			  "Voxels outside the new bounds are cut off. Each axis must stay at or below 512. "
			  "nodeUUID defaults to the active model. Y is up.");

	json::Json inputSchema = objectSchema();
	json::Json properties = inputSchema.get("properties");
	json::Json nodeProp = propUUID();
	nodeProp.set("description", "Model node UUID. Defaults to the active model node.");
	properties.set("nodeUUID", nodeProp);
	properties.set("size", propTypeDescription("array", "[x, y, z] voxel counts. The lower corner stays put."));
	properties.set("region", propTypeDescription("object", "Inclusive bounds minX, minY, minZ, maxX, maxY, maxZ"));
	json::Json fit = propTypeDescription("boolean", "Fit the volume to the voxels. Default false.");
	fit.set("default", false);
	properties.set("fit", fit);
	json::Json padding = propTypeDescription("integer", "Empty cells around the fitted voxels. Default 0.");
	padding.set("default", 0);
	properties.set("padding", padding);
	_tool.set("inputSchema", core::move(inputSchema));
}

bool ResizeVolumeTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	core::String error;
	scenegraph::SceneGraphNode *node = authoringNode(ctx, args, error);
	if (node == nullptr) {
		return ctx.result(id, error, true);
	}
	const voxel::RawVolume *volume = node->volume();
	const voxel::Region oldRegion = volume->region();
	voxel::Region target = voxel::Region::InvalidRegion;
	if (args.boolVal("fit", false)) {
		const int padding = args.intVal("padding", 0);
		if (padding < 0) {
			return ctx.result(id, "padding must be >= 0", true);
		}
		bool any = false;
		voxel::Region fitted(0, 0, 0, 0, 0, 0);
		voxelutil::visitVolume(*volume, [&fitted, &any](int x, int y, int z, const voxel::Voxel &) {
			if (!any) {
				fitted = voxel::Region(x, y, z, x, y, z);
				any = true;
				return;
			}
			fitted.accumulate(x, y, z);
		}, voxelutil::VisitSolid());
		if (!any) {
			return ctx.result(id, "Nothing to fit - the model has no voxels", true);
		}
		const math::Vec3i lo = fitted.getLowerCorner() - math::Vec3i(padding);
		const math::Vec3i hi = fitted.getUpperCorner() + math::Vec3i(padding);
		target = voxel::Region(lo, hi);
	} else if (args.contains("region")) {
		if (!parseRegionArg(args.get("region"), target, error)) {
			return ctx.result(id, error, true);
		}
	} else if (args.contains("size")) {
		math::Vec3i count(0);
		if (!parseCell(args.get("size"), count, "size", error)) {
			return ctx.result(id, error, true);
		}
		if (count.x < 1 || count.y < 1 || count.z < 1) {
			return ctx.result(id, "size components must be >= 1", true);
		}
		const math::Vec3i mins = oldRegion.getLowerCorner();
		target = voxel::Region(mins, mins + count - math::Vec3i(1));
	} else {
		return ctx.result(id, "Pass size, region, or fit", true);
	}
	if (!target.isValid()) {
		return ctx.result(id, "The new volume bounds are invalid", true);
	}
	if (axisTooLarge(target)) {
		return ctx.result(id, "Each axis must be 512 voxels or less", true);
	}
	const core::UUID uuid = node->uuid();
	ctx.sceneMgr->nodeResize(uuid, target);
	const voxel::RawVolume *resized = ctx.sceneMgr->volume(uuid);
	if (resized == nullptr || resized->region() != target) {
		return ctx.result(id, "Failed to resize the volume", true);
	}
	return ctx.result(id, core::String::format("Volume is now %s.", resized->region().toString().c_str()), false);
}

} // namespace voxedit
