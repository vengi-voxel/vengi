/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "core/String.h"
#include "core/UUID.h"
#include "core/collection/DynamicArray.h"
#include "json/JSON.h"
#include "network/ProtocolHandler.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxedit-util/network/ClientNetwork.h"
#include "voxedit-util/network/protocol/CommandsListMessage.h"
#include "voxedit-util/network/protocol/LuaScriptsListMessage.h"
#include "voxel/RawVolume.h"

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

/**
 * @brief MCP (Model Context Protocol) server for vengi
 * @ingroup Tools
 */
class McpServer : public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;

	friend class LuaScriptsListHandler;
	friend class CommandsListHandler;

	bool _initialized = false;
	voxedit::SceneRendererPtr _sceneRenderer;
	voxedit::ModifierRendererPtr _modifierRenderer;
	voxedit::SceneManagerPtr _sceneMgr;
	voxedit::ClientNetwork _network;
	LuaScriptsListHandler _luaScriptsListHandler;
	CommandsListHandler _commandsListHandler;

	core::DynamicArray<voxedit::LuaScriptInfo> _scripts;
	core::DynamicArray<voxedit::CommandInfo> _commands;
	bool _scriptsReceived = false;
	bool _commandsReceived = false;
	uint64_t _lastConnectionAttemptMillis = 0;

	bool connectToVoxEdit();
	void disconnectFromVoxEdit();
	bool sendCommand(const core::String &command);
	bool createLuaScript(const core::String &name, const core::String &content);
	bool sendVoxelModification(const core::UUID &nodeUUID, const voxel::RawVolume &volume,
							   const voxel::Region &region = voxel::Region::InvalidRegion);
	bool requestScripts();
	bool requestCommands();

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
