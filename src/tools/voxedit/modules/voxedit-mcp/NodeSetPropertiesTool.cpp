/**
 * @file
 */

#include "NodeSetPropertiesTool.h"
#include "core/UUID.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeSetPropertiesTool::NodeSetPropertiesTool() : Tool("voxedit_node_set_properties") {
	_tool.set("description", "Set arbitrary node properties. Provide nodeUUID and a properties object");

	json::Json inps = json::Json::object();
	inps.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	_requiredArr.push("properties");
	inps.set("required", _requiredArr);
	inps.get("properties").set("nodeUUID", propUUID());
	inps.get("properties").set("properties", propTypeDescription("object", "Properties map (string->string)"));
	_tool.set("inputSchema", core::move(inps));
}

bool NodeSetPropertiesTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	if (!args.contains("properties") || !args.get("properties").isObject()) {
		return ctx.result(id, "Missing properties", true);
	}
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}
	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found in scene graph - fetch the scene state first", true);
	}
	for (auto it = args.get("properties").begin(); it != args.get("properties").end(); ++it) {
		node->setProperty(it.key().c_str(), (*it).str().c_str());
	}
	return ctx.result(id, "Node properties updated successfully", false);
}

} // namespace voxedit
