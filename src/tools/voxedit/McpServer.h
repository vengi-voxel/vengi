/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "core/String.h"
#include "core/UUID.h"
#include "core/collection/DynamicArray.h"
#include "json/JSON.h"
#include "io/BufferedReadWriteStream.h"
#include "network/ProtocolHandler.h"
#include "network/ProtocolHandlerRegistry.h"
#include "network/ProtocolMessage.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/network/protocol/LuaScriptsListMessage.h"
#include "voxedit-util/network/protocol/CommandsListMessage.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"
#include "voxel/RawVolume.h"
#include "voxelgenerator/LUAApi.h"

namespace network {
struct NetworkImpl;
}

class McpServer;

class LuaScriptsListHandler : public network::ProtocolTypeHandler<voxedit::LuaScriptsListMessage> {
private:
	McpServer *_server;

public:
	LuaScriptsListHandler(McpServer *server) : _server(server) {
	}
	void execute(const network::ClientId &clientId, voxedit::LuaScriptsListMessage *message) override;
};

class CommandsListHandler : public network::ProtocolTypeHandler<voxedit::CommandsListMessage> {
private:
	McpServer *_server;

public:
	CommandsListHandler(McpServer *server) : _server(server) {
	}
	void execute(const network::ClientId &clientId, voxedit::CommandsListMessage *message) override;
};

class SceneStateHandler : public network::ProtocolTypeHandler<voxedit::SceneStateMessage> {
private:
	McpServer *_server;

public:
	SceneStateHandler(McpServer *server) : _server(server) {
	}
	void execute(const network::ClientId &clientId, voxedit::SceneStateMessage *message) override;
};

/**
 * @brief MCP (Model Context Protocol) server for vengi
 * @ingroup Tools
 */
class McpServer : public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;

	friend class LuaScriptsListHandler;
	friend class CommandsListHandler;
	friend class SceneStateHandler;

	bool _initialized = false;
	network::NetworkImpl *_network;
	network::MessageStream _inStream;
	network::ProtocolHandlerRegistry _protocolRegistry;
	network::NopHandler _nopHandler;
	LuaScriptsListHandler _luaScriptsListHandler;
	CommandsListHandler _commandsListHandler;
	SceneStateHandler _sceneStateHandler;
	core::String _host = "127.0.0.1";
	uint16_t _port = 10001;
	core::String _rconPassword = "changeme";
	core::String _connPassword = "changeme";
	voxelgenerator::LUAApi _luaApi;

	core::DynamicArray<voxedit::LuaScriptInfo> _scripts;
	core::DynamicArray<voxedit::CommandInfo> _commands;
	scenegraph::SceneGraph _sceneGraph;
	bool _scriptsReceived = false;
	bool _commandsReceived = false;
	bool _sceneStateReceived = false;

	bool connectToVoxEdit();
	void disconnectFromVoxEdit();
	bool sendMessage(const network::ProtocolMessage &msg);
	bool sendCommand(const core::String &command);
	bool createLuaScript(const core::String &name, const core::String &content);
	bool sendVoxelModification(const core::UUID &nodeUUID, const voxel::RawVolume &volume, const voxel::Region &region = voxel::Region::InvalidRegion);
	bool requestScripts();
	bool requestCommands();
	void processIncomingMessages();

	void handleRequest(const nlohmann::json &request);
	void handleInitialize(const nlohmann::json &request);
	void handleToolsList(const nlohmann::json &request);
	void handleToolsCall(const nlohmann::json &request);

	void sendResponse(const nlohmann::json &response);
	void sendError(const nlohmann::json &id, int code, const core::String &message);
	void sendToolResult(const nlohmann::json &id, const core::String &text, bool isError = false);

public:
	McpServer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onRunning() override;
	app::AppState onCleanup() override;
};
