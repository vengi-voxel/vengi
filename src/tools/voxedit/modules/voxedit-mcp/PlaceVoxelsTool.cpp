/**
 * @file
 */

#include "PlaceVoxelsTool.h"
#include "voxedit-util/network/protocol/VoxelModificationMessage.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"

namespace voxedit {

PlaceVoxelsTool::PlaceVoxelsTool() : Tool("voxedit_place_voxels") {
	_tool["description"] = "Place voxels at specified positions in a node.";

	nlohmann::json itemsSchema;
	itemsSchema["type"] = "object";
	itemsSchema["properties"]["x"]["type"] = "integer";
	itemsSchema["properties"]["y"]["type"] = "integer";
	itemsSchema["properties"]["z"]["type"] = "integer";
	itemsSchema["properties"]["colorIndex"]["type"] = "integer";
	itemsSchema["required"] = nlohmann::json::array({"x", "y", "z", "colorIndex"});

	nlohmann::json voxelsProp = propTypeDescription("array", "Array of {x, y, z, colorIndex} objects");
	voxelsProp["items"] = core::move(itemsSchema);

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"voxels", "nodeUUID"});
	inputSchema["properties"]["voxels"] = core::move(voxelsProp);
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

	// TODO: MCP: use the sceneMgr here to keep the state of the scenegraph in sync and don't operate directly with the protocol messages
	voxel::SparseVolume volume;
	int placedCount = 0;
	for (const auto &voxelData : voxelsArray) {
		const int x = voxelData["x"].get<int>();
		const int y = voxelData["y"].get<int>();
		const int z = voxelData["z"].get<int>();
		const int colorIndex = voxelData.value("colorIndex", 1);
		if (colorIndex > 0 && colorIndex < 256) {
			volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, colorIndex));
			++placedCount;
		}
	}

	voxel::RawVolume rawVolume(volume.calculateRegion());
	volume.copyTo(rawVolume);
	voxedit::VoxelModificationMessage msg(nodeUUID, rawVolume, rawVolume.region());
	if (sendMessage(ctx, msg)) {
		return ctx.result(id, core::String::format("Placed %d voxels in node %s", placedCount, nodeUUID.str().c_str()),
						  false);
	}
	return ctx.result(id, "Failed to send voxel modification", true);
}

} // namespace voxedit
