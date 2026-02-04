/**
 * @file
 */

#include "NodeRemoveTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeRemoveTool::NodeRemoveTool() : Tool("voxedit_node_remove") {
	_tool["description"] = "Remove a scene node by UUID";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID"});
	inputSchema["properties"]["nodeUUID"] = propUUID();
	_tool["inputSchema"] = core::move(inputSchema);
}

bool NodeRemoveTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
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
