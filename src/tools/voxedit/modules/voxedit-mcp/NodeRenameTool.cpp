/**
 * @file
 */

#include "NodeRenameTool.h"
#include "core/UUID.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeRenameTool::NodeRenameTool() : Tool("voxedit_node_rename") {
	_tool.set("description", "Rename a scene node by UUID");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	_requiredArr.push("name");
	inputSchema.set("required", _requiredArr);
	inputSchema.get("properties").set("nodeUUID", propUUID());
	inputSchema.get("properties").set("name", propTypeDescription("string", "New name for the node"));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool NodeRenameTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}
	const core::String &newName = args.get("name").str().c_str();
	const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found in scene graph - fetch the scene state first", true);
	}
	const int nodeId = node->id();
	if (ctx.sceneMgr->nodeRename(nodeId, newName)) {
		return ctx.result(id, core::String::format("Renamed node %s to %s", nodeUUID.str().c_str(), newName.c_str()),
						  false);
	}
	return ctx.result(id, "Failed to rename node - fetch the scene state first", true);
}

} // namespace voxedit