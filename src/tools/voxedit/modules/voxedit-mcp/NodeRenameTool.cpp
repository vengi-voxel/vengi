/**
 * @file
 */

#include "NodeRenameTool.h"
#include "core/UUID.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeRenameTool::NodeRenameTool() : Tool("voxedit_node_rename") {
	_tool["description"] = "Rename a scene node by UUID";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID", "name"});
	inputSchema["properties"]["nodeUUID"] = propUUID();
	inputSchema["properties"]["name"] = propTypeDescription("string", "New name for the node");
	_tool["inputSchema"] = core::move(inputSchema);
}

bool NodeRenameTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID", true);
	}
	const core::String &newName = args["name"].get<std::string>().c_str();
	const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found in scene graph", true);
	}
	const int nodeId = node->id();
	if (ctx.sceneMgr->nodeRename(nodeId, newName)) {
		return ctx.result(id, core::String::format("Renamed node %s to %s", nodeUUID.str().c_str(), newName.c_str()),
						  false);
	}
	return ctx.result(id, "Failed to rename node", true);
}

} // namespace voxedit