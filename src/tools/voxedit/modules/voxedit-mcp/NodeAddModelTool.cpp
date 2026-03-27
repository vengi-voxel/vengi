/**
 * @file
 */

#include "NodeAddModelTool.h"
#include "memento/MementoHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"

namespace voxedit {

// TODO: MCP: not only model nodes, but all other node types, too
NodeAddModelTool::NodeAddModelTool() : Tool("voxedit_node_add_model") {
	_tool.set("description", "Create a new model node. Provide parent UUID from the scene graph state, name and optinally the size");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("parentUUID");
	_requiredArr.push("name");
	inputSchema.set("required", _requiredArr);
	inputSchema.get("properties").set("parentUUID", propParentUUID());
	inputSchema.get("properties").set("name", propTypeDescription("string", "Name of the new node"));
	inputSchema.get("properties").set("width", propTypeDescription("integer", "Width of the model node volume region"));
	inputSchema.get("properties").get("width").set("default", 32);
	inputSchema.get("properties").set("height", propTypeDescription("integer", "Height of the model node volume region"));
	inputSchema.get("properties").get("height").set("default", 32);
	inputSchema.get("properties").set("depth", propTypeDescription("integer", "Depth of the model node volume region"));
	inputSchema.get("properties").get("depth").set("default", 32);
	_tool.set("inputSchema", core::move(inputSchema));
}

bool NodeAddModelTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID parentUUID = argsParentUUID(args);
	if (!parentUUID.isValid()) {
		return ctx.result(id, "Invalid parent UUID - fetch the scene state first", true);
	}
	const core::String name = args.strVal("name", "newnode").c_str();
	const int w = args.intVal("width", 32);
	const int h = args.intVal("height", 32);
	const int d = args.intVal("depth", 32);
	if (w <= 0 || h <= 0 || d <= 0) {
		return ctx.result(id, "Invalid dimensions", true);
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(name);
	voxel::Region region(0, 0, 0, w - 1, h - 1, d - 1);
	node.setVolume(new voxel::RawVolume(region));

	int parentId = 0;
	if (const scenegraph::SceneGraphNode *parentNode = ctx.sceneMgr->sceneGraphNodeByUUID(parentUUID)) {
		parentId = parentNode->id();
	}

	int nodeId = ctx.sceneMgr->moveNodeToSceneGraph(node, parentId);
	if (nodeId == InvalidNodeId) {
		return ctx.result(id, "Failed to add node to scene graph - fetch the scene state first", true);
	}
	return ctx.result(id, core::String::format("Added model node %s with id %i", node.uuid().str().c_str(), nodeId),
					  false);
}

} // namespace voxedit
