/**
 * @file
 */

#include "Tool.h"
#include "network/ProtocolMessage.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

Tool::Tool(const core::String &name) : _name(name) {
	_tool = json::Json::object();
	_tool.set("name", _name.c_str());
}

bool Tool::sendMessage(ToolContext &ctx, const network::ProtocolMessage &msg) {
	Client &client = ctx.sceneMgr->client();
	ClientNetwork &network = client.network();
	return network.sendMessage(msg);
}

json::Json Tool::propVoxels() {
	json::Json itemsSchema = json::Json::object();
	itemsSchema.set("type", "object");
	json::Json itemProps = json::Json::object();
	itemProps.set("x", propTypeDescription("integer", "X coordinate of the voxel"));
	itemProps.set("y", propTypeDescription("integer", "Y coordinate of the voxel"));
	itemProps.set("z", propTypeDescription("integer", "Z coordinate of the voxel"));
	itemProps.set("idx", propTypeDescription("integer", "Node palette color index of the voxel"));
	itemsSchema.set("properties", itemProps);

	json::Json voxelsProp = propTypeDescription("array", "Array of {x, y, z, colorIndex} objects there y is pointing upwards.");
	voxelsProp.set("items", itemsSchema);
	return voxelsProp;
}

json::Json Tool::propUUID() {
	return propTypeDescription("string", "UUID of the scene graph node - fetch the scene state to get the UUIDs of existing nodes");
}

json::Json Tool::propParentUUID() {
	return propTypeDescription("string", "UUID of the new parent node - fetch the scene state to get the UUIDs of existing nodes");
}

json::Json Tool::propReferenceUUID() {
	return propTypeDescription("string", "UUID of the referenced node if the node type is a ModelReference - fetch the scene state to get the UUIDs of existing nodes");
}

json::Json Tool::propTypeDescription(const core::String &type, const core::String &description) {
	json::Json nameProp = json::Json::object();
	nameProp.set("type", type.c_str());
	nameProp.set("description", description.c_str());
	return nameProp;
}

const core::String &Tool::rconPassword() const {
	return core::getVar(cfg::VoxEditNetRconPassword)->strVal();
}

core::UUID Tool::argsUUID(const json::Json &args) const {
	if (args.contains("nodeUUID") && args.get("nodeUUID").isString()) {
		return core::UUID(args.get("nodeUUID").cStr());
	}
	return {};
}

core::UUID Tool::argsParentUUID(const json::Json &args) const {
	if (args.contains("parentUUID") && args.get("parentUUID").isString()) {
		return core::UUID(args.get("parentUUID").cStr());
	}
	return {};
}

core::UUID Tool::argsReferenceUUID(const json::Json &args) const {
	if (args.contains("referenceUUID") && args.get("referenceUUID").isString()) {
		return core::UUID(args.get("referenceUUID").cStr());
	}
	return {};
}

bool Tool::sendCommand(ToolContext &ctx, const core::String &cmd, const json::Json &id) {
	if (sendMessage(ctx, voxedit::CommandMessage(cmd, rconPassword()))) {
		return ctx.result(id, core::String::format("Executed: %s", cmd.c_str()), false);
	}
	return ctx.result(id, core::String::format("Failed to send %s command", cmd.c_str()), true);
}

} // namespace voxedit
