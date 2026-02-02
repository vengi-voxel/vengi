/**
 * @file
 */

#include "Tool.h"
#include "network/ProtocolMessage.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

bool Tool::sendMessage(ToolContext &ctx, const network::ProtocolMessage &msg) {
	Client &client = ctx.sceneMgr->client();
	ClientNetwork &network = client.network();
	return network.sendMessage(msg);
}

nlohmann::json Tool::propUUID() {
	nlohmann::json nodeUUIDProp;
	nodeUUIDProp["type"] = "string";
	nodeUUIDProp["description"] = "UUID of the scene graph node";
	return nodeUUIDProp;
}

nlohmann::json Tool::propParentUUID() {
	nlohmann::json parentUUIDProp;
	parentUUIDProp["type"] = "string";
	parentUUIDProp["description"] = "UUID of the new parent node";
	return parentUUIDProp;
}

nlohmann::json Tool::propReferenceUUID() {
	nlohmann::json referenceUUIDProp;
	referenceUUIDProp["type"] = "string";
	referenceUUIDProp["description"] = "UUID of the reference sibling (optional)";
	return referenceUUIDProp;
}

nlohmann::json Tool::propTypeDescription(const core::String &type, const core::String &description) {
	nlohmann::json nameProp;
	nameProp["type"] = type.c_str();
	nameProp["description"] = description.c_str();
	return nameProp;
}

const core::String &Tool::rconPassword() const {
	return core::Var::getSafe(cfg::VoxEditNetRconPassword)->strVal();
}

core::UUID Tool::argsUUID(const nlohmann::json &args) const {
	const core::UUID nodeUUID(args["nodeUUID"].get<std::string>().c_str());
	return nodeUUID;
}

core::UUID Tool::argsParentUUID(const nlohmann::json &args) const {
	const core::UUID nodeUUID(args["parentUUID"].get<std::string>().c_str());
	return nodeUUID;
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
