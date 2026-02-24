/**
 * @file
 */

#include "Tool.h"
#include "network/ProtocolMessage.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

Tool::Tool(const core::String &name) : _name(name) {
	// TODO: MCP: sanitize name
	// letters (A-Z, a-z), digits (0-9), underscore (_), hyphen (-), and dot (.)
	// may NOT contain spaces, commas, or other special characters.
	_tool["name"] = _name.c_str();
	// TODO: MCP: _tool["title"]
}

bool Tool::sendMessage(ToolContext &ctx, const network::ProtocolMessage &msg) {
	Client &client = ctx.sceneMgr->client();
	ClientNetwork &network = client.network();
	return network.sendMessage(msg);
}

nlohmann::json Tool::propVoxels() {
	nlohmann::json itemsSchema;
	itemsSchema["type"] = "object";
	itemsSchema["properties"]["x"] = propTypeDescription("integer", "X coordinate of the voxel");
	itemsSchema["properties"]["y"] = propTypeDescription("integer", "Y coordinate of the voxel");
	itemsSchema["properties"]["z"] = propTypeDescription("integer", "Z coordinate of the voxel");
	itemsSchema["properties"]["idx"] = propTypeDescription("integer", "Node palette color index of the voxel");

	nlohmann::json voxelsProp = propTypeDescription("array", "Array of {x, y, z, colorIndex} objects there y is pointing upwards.");
	voxelsProp["items"] = core::move(itemsSchema);
	return voxelsProp;
}

nlohmann::json Tool::propUUID() {
	return propTypeDescription("string", "UUID of the scene graph node - fetch the scene state to get the UUIDs of existing nodes");
}

nlohmann::json Tool::propParentUUID() {
	return propTypeDescription("string", "UUID of the new parent node - fetch the scene state to get the UUIDs of existing nodes");
}

nlohmann::json Tool::propReferenceUUID() {
	return propTypeDescription("string", "UUID of the referenced node if the node type is a ModelReference - fetch the scene state to get the UUIDs of existing nodes");
}

nlohmann::json Tool::propTypeDescription(const core::String &type, const core::String &description) {
	nlohmann::json nameProp;
	nameProp["type"] = type.c_str();
	nameProp["description"] = description.c_str();
	return nameProp;
}

const core::String &Tool::rconPassword() const {
	return core::Var::getVar(cfg::VoxEditNetRconPassword)->strVal();
}

core::UUID Tool::argsUUID(const nlohmann::json &args) const {
	if (args.contains("nodeUUID") && args["nodeUUID"].is_string()) {
		return core::UUID(args["nodeUUID"].get<std::string>().c_str());
	}
	return {};
}

core::UUID Tool::argsParentUUID(const nlohmann::json &args) const {
	if (args.contains("parentUUID") && args["parentUUID"].is_string()) {
		return core::UUID(args["parentUUID"].get<std::string>().c_str());
	}
	return {};
}

core::UUID Tool::argsReferenceUUID(const nlohmann::json &args) const {
	if (args.contains("referenceUUID") && args["referenceUUID"].is_string()) {
		return core::UUID(args["referenceUUID"].get<std::string>().c_str());
	}
	return {};
}

bool Tool::sendCommand(ToolContext &ctx, const core::String &cmd, const nlohmann::json &id) {
	if (sendMessage(ctx, voxedit::CommandMessage(cmd, rconPassword()))) {
		return ctx.result(id, core::String::format("Executed: %s", cmd.c_str()), false);
	}
	return ctx.result(id, core::String::format("Failed to send %s command", cmd.c_str()), true);
}

} // namespace voxedit
