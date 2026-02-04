/**
 * @file
 */

#include "PlaceVoxelsTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"

namespace voxedit {

PlaceVoxelsTool::PlaceVoxelsTool() : Tool("voxedit_place_voxels") {
	_tool["description"] = "Place voxels at specified positions in a node.";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"voxels", "nodeUUID"});
	inputSchema["properties"]["voxels"] = propVoxels();
	inputSchema["properties"]["nodeUUID"] = propUUID();
	_tool["inputSchema"] = core::move(inputSchema);
}

bool PlaceVoxelsTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID", true);
	}

	if (!args.contains("voxels")) {
		return ctx.result(id, "Missing voxels argument", true);
	}
	const nlohmann::json &voxelsArray = args["voxels"];
	if (!voxelsArray.is_array() || voxelsArray.empty()) {
		return ctx.result(id, "voxels must be a non-empty array", true);
	}

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found", true);
	}
	const int nodeId = node->id();
	voxel::RawVolume *v = ctx.sceneMgr->volume(nodeId);
	if (v == nullptr) {
		return ctx.result(id, "Volume not found", true);
	}

	int placedCount = 0;
	voxel::Region modifiedRegion = voxel::Region::InvalidRegion;
	for (const auto &voxelData : voxelsArray) {
		const int x = voxelData["x"].get<int>();
		const int y = voxelData["y"].get<int>();
		const int z = voxelData["z"].get<int>();
		const int colorIndex = voxelData.value("idx", 1);
		if (colorIndex > 0 && colorIndex < 256) {
			if (v->setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, colorIndex))) {
				if (modifiedRegion.isValid()) {
					modifiedRegion.accumulate(x, y, z);
				} else {
					modifiedRegion = voxel::Region(x, y, z, x, y, z);
				}
				++placedCount;
			}
		}
	}

	if (placedCount > 0) {
		ctx.sceneMgr->modified(nodeId, modifiedRegion);
	}
	return ctx.result(id, core::String::format("Placed %d voxels in node %s", placedCount, nodeUUID.str().c_str()),
					  false);
}

} // namespace voxedit
