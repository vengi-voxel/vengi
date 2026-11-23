/**
 * @file
 */

#include "Tool.h"
#include "app/App.h"
#include "core/Log.h"
#include "network/ProtocolHandler.h"
#include "network/ProtocolHandlerRegistry.h"
#include "network/ProtocolMessage.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/network/ClientNetwork.h"
#include "voxedit-util/network/ProtocolIds.h"
#include "voxedit-util/network/protocol/CommandMessage.h"
#include "voxedit-util/network/protocol/LogMessage.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace voxedit {

namespace {

class LogCollector : public network::ProtocolTypeHandler<LogMessage> {
public:
	core::DynamicArray<core::String> logs;
	bool hadErrors = false;

	void execute(const network::ClientId &, LogMessage *message) override {
		const char *prefix;
		switch (message->level()) {
		case Log::Level::Error:
			prefix = "ERROR";
			hadErrors = true;
			break;
		case Log::Level::Warn:
			prefix = "WARN";
			break;
		case Log::Level::Info:
			prefix = "INFO";
			break;
		default:
			prefix = "DEBUG";
			break;
		}
		logs.push_back(core::String::format("server %s: %s", prefix, message->message().c_str()));
	}
};

class AckWaiter : public network::ProtocolHandler {
public:
	bool received = false;

	void execute(const network::ClientId &, network::ProtocolMessage &) override {
		received = true;
	}
};

} // namespace

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
	json::Json objectItem = json::Json::object();
	objectItem.set("type", "object");
	json::Json itemProps = json::Json::object();
	itemProps.set("x", propTypeDescription("integer", "X coordinate of the voxel"));
	itemProps.set("y", propTypeDescription("integer", "Y coordinate of the voxel (up)"));
	itemProps.set("z", propTypeDescription("integer", "Z coordinate of the voxel"));
	itemProps.set("idx", propTypeDescription("integer", "Palette index. 0 is a real slot. Ignored when color is set."));
	json::Json colorProp = json::Json::object();
	colorProp.set("description", "#RRGGBB or a palette index. -1 erases. A new hex color is added to the palette.");
	itemProps.set("color", colorProp);
	objectItem.set("properties", itemProps);

	json::Json arrayItem = json::Json::object();
	arrayItem.set("type", "array");
	arrayItem.set("minItems", 4);
	arrayItem.set("maxItems", 4);
	arrayItem.set("description", "[x, y, z, color]. color is #RRGGBB or a palette index. -1 erases.");

	json::Json oneOf = json::Json::array();
	oneOf.push(core::move(objectItem));
	oneOf.push(core::move(arrayItem));
	json::Json items = json::Json::object();
	items.set("oneOf", core::move(oneOf));

	json::Json voxelsProp = propTypeDescription(
		"array", "Voxels as {x, y, z, idx}, {x, y, z, color} or [x, y, z, color]. Y is up. At most 50000 per call.");
	voxelsProp.set("items", core::move(items));
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

json::Json Tool::objectSchema() {
	json::Json schema = json::Json::object();
	schema.set("type", "object");
	schema.set("properties", json::Json::object());
	return schema;
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

bool Tool::waitForServerResponse(ToolContext &ctx, core::DynamicArray<core::String> &logs, bool &hadErrors) {
	Client &client = ctx.sceneMgr->client();
	ClientNetwork &network = client.network();
	network::ProtocolHandlerRegistry &registry = network.protocolRegistry();

	// Save original handlers
	network::ProtocolHandler *origLogHandler = registry.getHandler(PROTO_LOG);
	network::ProtocolHandler *origAckHandler = registry.getHandler(PROTO_ACK);

	// Install temporary handlers to collect server log output and detect ack
	LogCollector logCollector;
	AckWaiter ackWaiter;
	registry.registerHandler(PROTO_LOG, &logCollector);
	registry.registerHandler(PROTO_ACK, &ackWaiter);

	// Poll network until ack received or timeout (5 seconds)
	static constexpr int maxIterations = 500;
	static constexpr int sleepMs = 10;
	for (int i = 0; i < maxIterations && !ackWaiter.received; ++i) {
		network.update(0.0);
		if (!ackWaiter.received) {
			app::App::getInstance()->wait(sleepMs);
		}
	}

	// Restore original handlers
	if (origLogHandler) {
		registry.registerHandler(PROTO_LOG, origLogHandler);
	}
	if (origAckHandler) {
		registry.registerHandler(PROTO_ACK, origAckHandler);
	}

	logs = core::move(logCollector.logs);
	hadErrors = logCollector.hadErrors;
	return ackWaiter.received;
}

bool Tool::sendCommand(ToolContext &ctx, const core::String &cmd, const json::Json &id) {
	if (!sendMessage(ctx, voxedit::CommandMessage(cmd, rconPassword()))) {
		return ctx.result(id, core::String::format("Failed to send %s command", cmd.c_str()), true);
	}

	core::DynamicArray<core::String> logs;
	bool hadErrors = false;
	const bool ackReceived = waitForServerResponse(ctx, logs, hadErrors);

	core::String result;
	if (ackReceived) {
		result = core::String::format("Executed: %s", cmd.c_str());
	} else {
		result = core::String::format("Executed: %s (timeout waiting for server response)", cmd.c_str());
	}

	for (const core::String &log : logs) {
		result.append("\n");
		result.append(log);
	}

	return ctx.result(id, result, hadErrors);
}

} // namespace voxedit
