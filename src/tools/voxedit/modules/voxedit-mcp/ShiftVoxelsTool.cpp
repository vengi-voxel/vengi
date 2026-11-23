/**
 * @file
 */

#include "ShiftVoxelsTool.h"
#include "Authoring.h"

namespace voxedit {

ShiftVoxelsTool::ShiftVoxelsTool() : Tool("voxedit_shift_voxels") {
	_tool.set("description",
			  "Shift every voxel of a model node by [dx, dy, dz]. Voxels that leave the volume are dropped. "
			  "nodeUUID defaults to the active model. Y is up. Grow the volume with voxedit_resize_volume first "
			  "when the shift would leave the canvas.");

	json::Json inputSchema = objectSchema();
	json::Json required = json::Json::array();
	required.push("offset");
	inputSchema.set("required", required);
	json::Json properties = inputSchema.get("properties");
	json::Json nodeProp = propUUID();
	nodeProp.set("description", "Model node UUID. Defaults to the active model node.");
	properties.set("nodeUUID", nodeProp);
	properties.set("offset", propTypeDescription("array", "[dx, dy, dz] shift in voxels"));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool ShiftVoxelsTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	core::String error;
	scenegraph::SceneGraphNode *node = authoringNode(ctx, args, error);
	if (node == nullptr) {
		return ctx.result(id, error, true);
	}
	if (!args.contains("offset")) {
		return ctx.result(id, "offset is required", true);
	}
	glm::ivec3 offset(0);
	if (!parseCell(args.get("offset"), offset, "offset", error)) {
		return ctx.result(id, error, true);
	}
	const voxel::RawVolume *volume = node->volume();
	core::DynamicArray<SolidVoxel> solids;
	if (!collectSolids(*volume, solids, error)) {
		return ctx.result(id, error, true);
	}
	core::DynamicArray<VoxelEdit> edits;
	for (const SolidVoxel &solid : solids) {
		VoxelEdit clear;
		clear.pos = solid.pos;
		clear.voxel = voxel::Voxel();
		edits.push_back(clear);
	}
	for (const SolidVoxel &solid : solids) {
		VoxelEdit write;
		write.pos = solid.pos + offset;
		write.voxel = solid.voxel;
		edits.push_back(write);
	}
	int changed = 0;
	int skipped = 0;
	if (!applyEdits(ctx, *node, edits, changed, skipped)) {
		return ctx.result(id, "Volume not found - this is no model node", true);
	}
	return ctx.result(id, core::String::format("Shifted by [%i, %i, %i]. %i voxels changed, %i landed outside the volume.",
											   offset.x, offset.y, offset.z, changed, skipped),
					  false);
}

} // namespace voxedit
