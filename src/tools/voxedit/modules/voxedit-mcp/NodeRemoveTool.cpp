/**
 * @file
 */

#include "NodeRemoveTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeRemoveTool::NodeRemoveTool() : Tool("voxedit_node_remove") {
	_tool.set("description", "Remove a scene node by UUID");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	inputSchema.set("required", _requiredArr);
	inputSchema.get("properties").set("nodeUUID", propUUID());
	_tool.set("inputSchema", core::move(inputSchema));
}

bool NodeRemoveTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}
	if (const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID)) {
		if (!ctx.sceneMgr->nodeRemove(node->id(), false)) {
			return ctx.result(id, "Failed to remove node from scene graph - fetch the scene state first", true);
		}
		return ctx.result(id, "Node removed successfully", false);
	}
	return ctx.result(id, "Node not found in scene graph - fetch the scene state first", true);
}

} // namespace voxedit
