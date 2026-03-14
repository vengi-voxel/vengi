/**
 * @file
 */

#include "PlaceVoxelsTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/RawVolume.h"

namespace voxedit {

PlaceVoxelsTool::PlaceVoxelsTool() : Tool("voxedit_place_voxels") {
	_tool.set("description", "Place voxels at specified positions in a node.");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("voxels");
	_requiredArr.push("nodeUUID");
	inputSchema.set("required", _requiredArr);
	inputSchema.get("properties").set("voxels", propVoxels());
	inputSchema.get("properties").set("nodeUUID", propUUID());
	_tool.set("inputSchema", core::move(inputSchema));
}

bool PlaceVoxelsTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	if (!args.contains("voxels")) {
		return ctx.result(id, "Missing voxels argument", true);
	}
	json::Json voxelsArray = args.get("voxels");
	if (!voxelsArray.isArray() || voxelsArray.empty()) {
		return ctx.result(id, "voxels must be a non-empty array", true);
	}

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found - fetch the scene state first", true);
	}
	const int nodeId = node->id();
	voxel::RawVolume *v = ctx.sceneMgr->volume(nodeId);
	if (v == nullptr) {
		return ctx.result(id, "Volume not found - fetch the scene state first, this is no model node", true);
	}

	ModifierVolumeWrapper wrapper(*node, ModifierType::Override);
	for (const auto &voxelData : voxelsArray) {
		const int x = voxelData.get("x").intVal();
		const int y = voxelData.get("y").intVal();
		const int z = voxelData.get("z").intVal();
		const int colorIndex = voxelData.intVal("idx", 1);
		if (!wrapper.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, colorIndex))) {
			return ctx.result(id, core::String::format("Failed to set voxel with colorIndex %i at %i,%i,%i", colorIndex, x, y, z), true);
		}
	}

	const voxel::Region &region = wrapper.dirtyRegion();
	if (region.isValid()) {
		ctx.sceneMgr->modified(nodeId, region);
		return ctx.result(id, "Voxels placed successfully", false);
	}
	return ctx.result(id, "No voxels were placed", true);
}

} // namespace voxedit
