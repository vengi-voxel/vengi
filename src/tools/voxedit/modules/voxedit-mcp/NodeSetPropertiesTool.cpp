/**
 * @file
 */

#include "NodeSetPropertiesTool.h"
#include "core/UUID.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

NodeSetPropertiesTool::NodeSetPropertiesTool() : Tool("voxedit_node_set_properties") {
	_tool["description"] = "Set arbitrary node properties. Provide nodeUUID and a properties object";

	nlohmann::json inps;
	inps["type"] = "object";
	inps["required"] = nlohmann::json::array({"nodeUUID", "properties"});
	inps["properties"]["nodeUUID"] = propUUID();
	inps["properties"]["properties"] = propTypeDescription("object", "Properties map (string->string)");
	_tool["inputSchema"] = core::move(inps);
}

bool NodeSetPropertiesTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	if (!args.contains("properties") || !args["properties"].is_object()) {
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
	for (auto it = args["properties"].begin(); it != args["properties"].end(); ++it) {
		node->setProperty(it.key().c_str(), it.value().get<std::string>().c_str());
	}
	return ctx.result(id, "Node properties updated successfully", false);
}

} // namespace voxedit
