/**
 * @file
 */

#include "NodeMoveTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeMoveTool::NodeMoveTool() : Tool("voxedit_node_move") {
	_tool.set("description", "Move a node to a new parent (and optional reference)");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	_requiredArr.push("parentUUID");
	inputSchema.set("required", _requiredArr);
	inputSchema.get("properties").set("nodeUUID", Tool::propUUID());
	inputSchema.get("properties").set("parentUUID", Tool::propParentUUID());
	_tool.set("inputSchema", core::move(inputSchema));
}

bool NodeMoveTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}
	const core::UUID parentUUID = argsParentUUID(args);
	if (!parentUUID.isValid()) {
		return ctx.result(id, "Invalid parent UUID - fetch the scene state first", true);
	}
	const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found in scene graph - fetch the scene state first", true);
	}
	const scenegraph::SceneGraphNode *parentNode = ctx.sceneMgr->sceneGraphNodeByUUID(parentUUID);
	if (parentNode == nullptr) {
		return ctx.result(id, "Parent node not found in scene graph - fetch the scene state first", true);
	}
	if (!ctx.sceneMgr->nodeMove(node->id(), parentNode->id(), scenegraph::NodeMoveFlag::UpdateTransform)) {
		return ctx.result(id, "Failed to move node in scene graph - fetch the scene state first", true);
	}
	return ctx.result(id, "Node moved successfully", false);
}

} // namespace voxedit
