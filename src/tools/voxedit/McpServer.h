/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "network/ProtocolHandler.h"
#include "voxedit-mcp/ToolRegistry.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/IModifierRenderer.h"
#include "voxedit-util/network/protocol/LuaScriptsListMessage.h"
#include "json/JSON.h"

class McpServer;

class LuaScriptsListHandler : public network::ProtocolTypeHandler<voxedit::LuaScriptsListMessage> {
private:
	McpServer *_server;

public:
	LuaScriptsListHandler(McpServer *server) : _server(server) {
	}
	void execute(const network::ClientId &clientId, voxedit::LuaScriptsListMessage *message) override;
};

/**
 * @brief MCP (Model Context Protocol) server for vengi voxedit. It connects to a VoxEdit instance as normal client and
 * knows the whole scene state,
 * @ingroup Tools
 *
 * @li https://modelcontextprotocol.io/docs/tools/inspector
 *
 * @code npx @modelcontextprotocol/inspector path/to/vengi-voxeditmcp @endcode
 */
class McpServer : public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;

	friend class LuaScriptsListHandler;

	voxedit::SceneRendererPtr _sceneRenderer;
	voxedit::ModifierRendererPtr _modifierRenderer;
	voxedit::SceneManagerPtr _sceneMgr;
	LuaScriptsListHandler _luaScriptsListHandler;
	voxedit::ToolRegistry _toolRegistry;

	bool _initialized = false;

	uint64_t _lastConnectionAttemptMillis = 0;

	void printUsageHeader() const override;
	void usage() const override;

	bool requestScripts();
	void updateScriptTools(const core::DynamicArray<voxedit::LuaScriptInfo> &scripts);

	bool connectToVoxEdit();
	void disconnectFromVoxEdit();

	/** @brief Read JSON-RPC from stdin - only line at a time */
	bool handleStdin();
	void handleRequest(const nlohmann::json &request);
	void handleInitialize(const nlohmann::json &request);
	void handleToolsList(const nlohmann::json &request);
	void handleToolsCall(const nlohmann::json &request);

	static void sendResponse(const nlohmann::json &response);
	static void sendError(const nlohmann::json &id, int code, const core::String &message);
	static bool sendToolResult(const nlohmann::json &id, const core::String &text, bool isError = false);

public:
	McpServer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onRunning() override;
	app::AppState onCleanup() override;
};
