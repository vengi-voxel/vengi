/**
 * @file
 */

#include "NodeMoveTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeMoveTool::NodeMoveTool() : Tool("voxedit_node_move") {
	_tool["description"] = "Move a node to a new parent (and optional reference)";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID", "parentUUID"});
	inputSchema["properties"]["nodeUUID"] = Tool::propUUID();
	inputSchema["properties"]["parentUUID"] = Tool::propParentUUID();
	_tool["inputSchema"] = core::move(inputSchema);
}

bool NodeMoveTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID", true);
	}
	const core::UUID parentUUID = argsParentUUID(args);
	if (!parentUUID.isValid()) {
		return ctx.result(id, "Invalid parent UUID", true);
	}
	const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found in scene graph", true);
	}
	const scenegraph::SceneGraphNode *parentNode = ctx.sceneMgr->sceneGraphNodeByUUID(parentUUID);
	if (parentNode == nullptr) {
		return ctx.result(id, "Parent node not found in scene graph", true);
	}
	if (!ctx.sceneMgr->nodeMove(node->id(), parentNode->id(), scenegraph::NodeMoveFlag::UpdateTransform)) {
		return ctx.result(id, "Failed to move node in scene graph", true);
	}
	return ctx.result(id, "Node moved successfully", false);
}

} // namespace voxedit
