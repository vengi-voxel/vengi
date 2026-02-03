/**
 * @file
 */

#include "McpServer.h"
#include "command/Command.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "engine-config.h"
#include "engine-git.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Filesystem.h"
#include "voxedit-mcp/CommandTool.h"
#include "voxedit-mcp/FindColorTool.h"
#include "voxedit-mcp/GetPaletteTool.h"
#include "voxedit-mcp/GetSceneStateTool.h"
#include "voxedit-mcp/MementoCanRedoTool.h"
#include "voxedit-mcp/MementoCanUndoTool.h"
#include "voxedit-mcp/MementoRedoTool.h"
#include "voxedit-mcp/MementoUndoTool.h"
#include "voxedit-mcp/NodeAddModelTool.h"
#include "voxedit-mcp/NodeMoveTool.h"
#include "voxedit-mcp/NodeRemoveTool.h"
#include "voxedit-mcp/NodeRenameTool.h"
#include "voxedit-mcp/NodeSetPropertiesTool.h"
#include "voxedit-mcp/PlaceVoxelsTool.h"
#include "voxedit-mcp/ScriptApiTool.h"
#include "voxedit-mcp/ScriptCreateTool.h"
#include "voxedit-mcp/ScriptTool.h"
#include "voxedit-mcp/Tool.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/network/Client.h"
#include "voxedit-util/network/ClientNetwork.h"
#include "voxedit-util/network/ProtocolIds.h"
#include "voxedit-util/network/protocol/CommandsRequestMessage.h"
#include "voxedit-util/network/protocol/LuaScriptsRequestMessage.h"

// JSON-RPC error codes
static constexpr int PARSE_ERROR = -32700;
static constexpr int INVALID_REQUEST = -32600;
static constexpr int METHOD_NOT_FOUND = -32601;
static constexpr int INVALID_PARAMS = -32602;
static constexpr int INIT_FAILED = -32000;

void LuaScriptsListHandler::execute(const network::ClientId &clientId, voxedit::LuaScriptsListMessage *message) {
	_server->updateScriptTools(message->scripts());
}

void CommandsListHandler::execute(const network::ClientId &clientId, voxedit::CommandsListMessage *message) {
	_server->updateCommandTools(message->commands());
}

McpServer::McpServer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider, 1), _sceneRenderer(core::make_shared<voxedit::ISceneRenderer>()),
	  _modifierRenderer(core::make_shared<voxedit::IModifierRenderer>()),
	  _sceneMgr(core::make_shared<voxedit::SceneManager>(timeProvider, filesystem, _sceneRenderer, _modifierRenderer)),
	  _luaScriptsListHandler(this), _commandsListHandler(this) {
	Log::setConsoleColors(false);
	init(ORGANISATION, "vengimcp");
}

void McpServer::printUsageHeader() const {
	Super::printUsageHeader();
	Log::info("Git commit " GIT_COMMIT " - " GIT_COMMIT_DATE);
}

void McpServer::usage() const {
	Super::usage();
	Log::info("Registered tools:");

	for (const auto &toolPair : _toolRegistry.tools()) {
		const voxedit::Tool *tool = toolPair->second;
		Log::info(" - %s: %s", tool->name().c_str(), tool->inputSchema()["description"].get<std::string>().c_str());
	}

	usageFooter();
}

app::AppState McpServer::onConstruct() {
	const app::AppState state = Super::onConstruct();
	core::Var::get(cfg::UILastDirectory, filesystem()->homePath().c_str());
	core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");
	core::Var::get(cfg::ClientCameraZoomSpeed, "0.1");
	_sceneMgr->construct();

	command::Command::registerCommand("toollist", [this](const command::CmdArgs &args) {
		for (const auto &toolPair : _toolRegistry.tools()) {
			const voxedit::Tool *tool = toolPair->second;
			Log::info(" - %s: %s", tool->name().c_str(), tool->inputSchema()["description"].get<std::string>().c_str());
		}
		return 0;
	});

	// script and command tool are registered later
	_toolRegistry.registerTool(new voxedit::FindColorTool());
	_toolRegistry.registerTool(new voxedit::GetPaletteTool());
	_toolRegistry.registerTool(new voxedit::GetSceneStateTool());
	_toolRegistry.registerTool(new voxedit::MementoCanRedoTool());
	_toolRegistry.registerTool(new voxedit::MementoCanUndoTool());
	_toolRegistry.registerTool(new voxedit::MementoRedoTool());
	_toolRegistry.registerTool(new voxedit::MementoUndoTool());
	_toolRegistry.registerTool(new voxedit::NodeAddModelTool());
	_toolRegistry.registerTool(new voxedit::NodeMoveTool());
	_toolRegistry.registerTool(new voxedit::NodeRemoveTool());
	_toolRegistry.registerTool(new voxedit::NodeRenameTool());
	_toolRegistry.registerTool(new voxedit::NodeSetPropertiesTool());
	_toolRegistry.registerTool(new voxedit::PlaceVoxelsTool());
	_toolRegistry.registerTool(new voxedit::ScriptApiTool());
	_toolRegistry.registerTool(new voxedit::ScriptCreateTool());

	return state;
}

void McpServer::updateScriptTools(const core::DynamicArray<voxedit::LuaScriptInfo> &scripts) {
	Log::debug("Received %d lua scripts from server", (int)scripts.size());
	for (const voxedit::LuaScriptInfo &script : scripts) {
		_toolRegistry.unregisterTool(voxedit::ScriptTool::toolName(script));
		_toolRegistry.registerTool(new voxedit::ScriptTool(script));
	}
}

void McpServer::updateCommandTools(const core::DynamicArray<voxedit::CommandInfo> &commands) {
	Log::debug("Received %d commands from server", (int)commands.size());
	for (const voxedit::CommandInfo &cmd : commands) {
		_toolRegistry.unregisterTool(voxedit::CommandTool::toolName(cmd));
		_toolRegistry.registerTool(new voxedit::CommandTool(cmd));
	}
}

app::AppState McpServer::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!_sceneMgr->init()) {
		Log::error("Failed to initialize scene manager");
		return app::AppState::InitFailure;
	}

	// Register our custom handlers for lua scripts and commands list
	voxedit::ClientNetwork &network = _sceneMgr->client().network();
	network::ProtocolHandlerRegistry &r = network.protocolRegistry();
	r.registerHandler(voxedit::PROTO_LUA_SCRIPTS_LIST, &_luaScriptsListHandler);
	r.registerHandler(voxedit::PROTO_COMMANDS_LIST, &_commandsListHandler);

	return state;
}

app::AppState McpServer::onCleanup() {
	disconnectFromVoxEdit();
	_sceneMgr->shutdown();
	_toolRegistry.shutdown();
	return Super::onCleanup();
}

bool McpServer::connectToVoxEdit() {
	voxedit::Client &client = _sceneMgr->client();
	if (client.isConnected()) {
		return true;
	}
	Log::debug("Connecting to VoxEdit server...");

	const core::String host = core::Var::getSafe(cfg::VoxEditNetHostname)->strVal();
	const int port = core::Var::getSafe(cfg::VoxEditNetPort)->intVal();

	core::Var::getSafe(cfg::AppUserName)->setVal("mcp-client");

	if (!client.connect(host, port, false)) {
		Log::error("Failed to connect to %s:%i", host.c_str(), port);
		return false;
	}

	requestScripts();
	requestCommands();
	return true;
}

void McpServer::disconnectFromVoxEdit() {
	_sceneMgr->client().disconnect();
}

bool McpServer::requestScripts() {
	voxedit::LuaScriptsRequestMessage requestMsg;
	return _sceneMgr->client().network().sendMessage(requestMsg);
}

bool McpServer::requestCommands() {
	voxedit::CommandsRequestMessage requestMsg;
	return _sceneMgr->client().network().sendMessage(requestMsg);
}

app::AppState McpServer::onRunning() {
	const double nowSeconds = _timeProvider->tickSeconds();
	voxedit::Client &client = _sceneMgr->client();

	// Check if disconnected and reconnect with 5-second delay between attempts
	if (_initialized && !client.isConnected()) {
		const uint64_t now = _timeProvider->tickNow();
		if (now - _lastConnectionAttemptMillis >= 5000) {
			Log::info("Connection lost, attempting to reconnect...");
			_lastConnectionAttemptMillis = now;

			if (connectToVoxEdit()) {
				Log::info("Reconnected to VoxEdit server");
				requestScripts();
				requestCommands();
			} else {
				Log::warn("Failed to reconnect to VoxEdit server");
			}
		}
	}

	client.update(nowSeconds);
	if (!handleStdin()) {
		Log::info("Standard input closed, shutting down MCP server");
		requestQuit();
	}
	return app::AppState::Running;
}

bool McpServer::handleStdin() {
	io::BufferedReadWriteStream stream;
	if (!readInputLine(stream)) {
		return false;
	}
	if (stream.empty()) {
		return true;
	}
	Log::debug("Reading MCP request from stdin...");

	stream.seek(0);
	core::String lineStr;
	if (!stream.readString((int)stream.size(), lineStr)) {
		Log::error("Failed to read MCP request from stream");
		return true;
	}

	if (lineStr[0] == '\n' || lineStr[0] == '\0') {
		Log::debug("Received empty MCP request");
		return true;
	}

	const char *line = lineStr.c_str();
	if (!nlohmann::json::accept(line)) {
		sendError(nullptr, PARSE_ERROR, "Parse error");
		return true;
	}

	const nlohmann::json &request = nlohmann::json::parse(line);
	if (request.is_discarded()) {
		sendError(nullptr, PARSE_ERROR, "Parse error");
		return true;
	}

	handleRequest(request);
	return true;
}

void McpServer::handleRequest(const nlohmann::json &request) {
	auto id = request.value("id", nlohmann::json());
	if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0") {
		sendError(id, INVALID_REQUEST, "Invalid JSON-RPC version");
		return;
	}

	if (!request.contains("method") || !request["method"].is_string()) {
		sendError(id, INVALID_REQUEST, "Missing method");
		return;
	}

	const std::string &method = request["method"];
	Log::debug("Received MCP request for method %s", method.c_str());

	if (method == "initialize") {
		handleInitialize(request);
	} else if (method == "notifications/initialized") {
		Log::info("MCP client initialized");
	} else if (method == "tools/list") {
		if (!connectToVoxEdit()) {
			Log::error("Failed to connect to VoxEdit server at %s:%d",
					   core::Var::getSafe(cfg::VoxEditNetHostname)->strVal().c_str(),
					   core::Var::getSafe(cfg::VoxEditNetPort)->intVal());
			sendError(id, INIT_FAILED, "Failed to connect to VoxEdit server");
			return;
		}
		handleToolsList(request);
	} else if (method == "tools/call") {
		handleToolsCall(request);
	} else if (method == "ping") {
		nlohmann::json response;
		response["jsonrpc"] = "2.0";
		response["id"] = id;
		response["result"] = {};
		sendResponse(response);
	} else {
		sendError(id, METHOD_NOT_FOUND, "Method not found");
	}
}

void McpServer::handleInitialize(const nlohmann::json &request) {
	Log::info("Received initialize request");
	if (request.contains("params") && request["params"].contains("clientInfo")) {
		const auto &clientInfo = request["params"]["clientInfo"];
		core::String _clientName = "unknown";
		core::String _clientVersion = "unknown";
		if (clientInfo.contains("name") && clientInfo["name"].is_string()) {
			_clientName = clientInfo["name"].get<std::string>().c_str();
		}
		if (clientInfo.contains("version") && clientInfo["version"].is_string()) {
			_clientVersion = clientInfo["version"].get<std::string>().c_str();
		}
		Log::info("Client: %s (version %s)", _clientName.c_str(), _clientVersion.c_str());
	}
	_initialized = true;

	nlohmann::json result;
	result["protocolVersion"] = "2024-11-05";
	result["capabilities"]["tools"]["listChanged"] = true;
	result["serverInfo"]["name"] = appname().c_str();
	result["serverInfo"]["version"] = PROJECT_VERSION;

	nlohmann::json response;
	response["jsonrpc"] = "2.0";
	response["id"] = request.value("id", nlohmann::json());
	response["result"] = core::move(result);
	sendResponse(response);
}

void McpServer::handleToolsList(const nlohmann::json &request) {
	Log::info("Received tools list request");
	nlohmann::json tools = nlohmann::json::array();
	_toolRegistry.addRegisteredTools(tools);
	nlohmann::json result;
	result["tools"] = core::move(tools);

	nlohmann::json response;
	response["jsonrpc"] = "2.0";
	response["id"] = request.value("id", nlohmann::json());
	response["result"] = core::move(result);
	sendResponse(response);
}

void McpServer::handleToolsCall(const nlohmann::json &request) {
	if (!request.contains("params") || !request["params"].contains("name")) {
		sendError(request.value("id", nlohmann::json()), INVALID_PARAMS, "Missing tool name");
		return;
	}

	const std::string &toolName = request["params"]["name"];
	const auto &args = request["params"].value("arguments", nlohmann::json::object());
	const nlohmann::json &id = request["id"];

	Log::info("Received tool call for %s", toolName.c_str());

	voxedit::ToolContext ctx;
	ctx.sceneMgr = _sceneMgr.get();
	ctx.result = sendToolResult;
	if (!_toolRegistry.call(toolName.c_str(), id, args, ctx)) {
		sendError(id, INVALID_PARAMS, "Unknown tool");
	}
}

bool McpServer::sendToolResult(const nlohmann::json &id, const core::String &text, bool isError) {
	if (isError) {
		Log::warn("Tool result error: %s", text.c_str());
	}
	nlohmann::json result;
	result["content"] = nlohmann::json::array();
	nlohmann::json content;
	content["type"] = "text";
	content["text"] = text.c_str();
	result["content"].push_back(content);
	if (isError) {
		result["isError"] = true;
	}

	nlohmann::json response;
	response["jsonrpc"] = "2.0";
	response["id"] = id;
	response["result"] = core::move(result);
	sendResponse(response);
	return !isError;
}

void McpServer::sendResponse(const nlohmann::json &response) {
	const std::string out = response.dump();
	Log::debug("Sending MCP response: %s", out.c_str());
	core_assert(out.find('\n') == std::string::npos); // ensure single line output
	fprintf(stdout, "%s\n", out.c_str());
	fflush(stdout);
}

void McpServer::sendError(const nlohmann::json &id, int code, const core::String &message) {
	Log::warn("Sending error %d: %s", code, message.c_str());
	nlohmann::json response;
	response["jsonrpc"] = "2.0";
	response["id"] = id;
	response["error"]["code"] = code;
	response["error"]["message"] = message.c_str();
	sendResponse(response);
}

CONSOLE_APP(McpServer)
