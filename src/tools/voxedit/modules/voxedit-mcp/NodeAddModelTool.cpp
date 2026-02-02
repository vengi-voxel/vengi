/**
 * @file
 */

#include "NodeAddModelTool.h"
#include "memento/MementoHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"

namespace voxedit {

// TODO: not only model nodes, but all other node types, too
NodeAddModelTool::NodeAddModelTool() : Tool("voxedit_node_add_model") {
	_tool["description"] = "Create a new model node. Provide optional parent UUID, name and size";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["properties"]["parentUUID"] = propParentUUID();
	inputSchema["properties"]["name"] = propTypeDescription("string", "Name of the new node");
	inputSchema["properties"]["width"] = propTypeDescription("integer", "Width");
	inputSchema["properties"]["width"]["default"] = 16;
	inputSchema["properties"]["height"] = propTypeDescription("integer", "Height");
	inputSchema["properties"]["height"]["default"] = 16;
	inputSchema["properties"]["depth"] = propTypeDescription("integer", "Depth");
	inputSchema["properties"]["depth"]["default"] = 16;
	_tool["inputSchema"] = core::move(inputSchema);
}

bool NodeAddModelTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const core::UUID parentUUID = argsParentUUID(args);
	const core::String name = args.value("name", "newnode").c_str();
	const int w = args.value("width", 16);
	const int h = args.value("height", 16);
	const int d = args.value("depth", 16);
	if (w <= 0 || h <= 0 || d <= 0) {
		return ctx.result(id, "Invalid dimensions", true);
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(name);
	voxel::Region region(0, 0, 0, w - 1, h - 1, d - 1);
	node.setVolume(new voxel::RawVolume(region), true);

	int parentId = 0;
	if (const scenegraph::SceneGraphNode *parentNode = ctx.sceneMgr->sceneGraphNodeByUUID(parentUUID)) {
		parentId = parentNode->id();
	}

	int nodeId = ctx.sceneMgr->moveNodeToSceneGraph(node, parentId);
	if (nodeId == InvalidNodeId) {
		return ctx.result(id, "Failed to add node to scene graph", true);
	}
	return ctx.result(id, core::String::format("Added model node %s with id %i", node.uuid().str().c_str(), nodeId),
					  false);
}

} // namespace voxedit
