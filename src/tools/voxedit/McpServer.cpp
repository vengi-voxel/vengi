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
#include "voxedit-mcp/AnimationAddTool.h"
#include "voxedit-mcp/AnimationSetTool.h"
#include "voxedit-mcp/CommandTool.h"
#include "voxedit-mcp/FindColorTool.h"
#include "voxedit-mcp/GetPaletteTool.h"
#include "voxedit-mcp/GetSceneStateTool.h"
#include "voxedit-mcp/HistogramTool.h"
#include "voxedit-mcp/GetVoxelsTool.h"
#include "voxedit-mcp/LineBrushTool.h"
#include "voxedit-mcp/MementoCanRedoTool.h"
#include "voxedit-mcp/MementoCanUndoTool.h"
#include "voxedit-mcp/MementoRedoTool.h"
#include "voxedit-mcp/MementoUndoTool.h"
#include "voxedit-mcp/NodeAddKeyframeTool.h"
#include "voxedit-mcp/NodeAddModelTool.h"
#include "voxedit-mcp/NodeMoveTool.h"
#include "voxedit-mcp/NodeRemoveTool.h"
#include "voxedit-mcp/NodeRenameTool.h"
#include "voxedit-mcp/NodeSetPropertiesTool.h"
#include "voxedit-mcp/PaintBrushTool.h"
#include "voxedit-mcp/PlaceVoxelsTool.h"
#include "voxedit-mcp/PlaneBrushTool.h"
#include "voxedit-mcp/ScriptApiTool.h"
#include "voxedit-mcp/ScriptCreateTool.h"
#include "voxedit-mcp/ScriptTool.h"
#include "voxedit-mcp/SelectBrushTool.h"
#include "voxedit-mcp/ShapeBrushTool.h"
#include "voxedit-mcp/Tool.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/network/Client.h"
#include "voxedit-util/network/ClientNetwork.h"
#include "voxedit-util/network/ProtocolIds.h"
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

McpServer::McpServer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider, 1), _sceneRenderer(core::make_shared<voxedit::ISceneRenderer>()),
	  _modifierRenderer(core::make_shared<voxedit::IModifierRenderer>()),
	  _sceneMgr(core::make_shared<voxedit::SceneManager>(timeProvider, filesystem, _sceneRenderer, _modifierRenderer)),
	  _luaScriptsListHandler(this) {
	Log::setConsoleColors(false);
	init(ORGANISATION, "mcpserver");
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
		Log::info(" - %s: %s", tool->name().c_str(), tool->inputSchema().strVal("description", "").c_str());
	}

	usageFooter();
}

app::AppState McpServer::onConstruct() {
	const app::AppState state = Super::onConstruct();
	const core::VarDef uILastDirectory(cfg::UILastDirectory, filesystem()->homePath(), N_("Last Directory"),
									   N_("The last directory used for opening/saving files"));
	core::Var::registerVar(uILastDirectory);
	const core::VarDef clientMouseRotationSpeed(cfg::ClientMouseRotationSpeed, 0.01f, N_("Client Mouse Rotation Speed"),
												N_("The speed of mouse rotation for the client"));
	core::Var::registerVar(clientMouseRotationSpeed);
	const core::VarDef clientCameraZoomSpeed(cfg::ClientCameraZoomSpeed, 0.1f, N_("Client Camera Zoom Speed"),
											 N_("The speed of camera zoom for the client"));
	core::Var::registerVar(clientCameraZoomSpeed);
	_sceneMgr->construct();

	command::Command::registerCommand("toollist")
		.setHandler([this](const command::CommandArgs &args) {
			for (const auto &toolPair : _toolRegistry.tools()) {
				const voxedit::Tool *tool = toolPair->second;
				Log::info(" - %s: %s", tool->name().c_str(), tool->inputSchema().strVal("description", "").c_str());
			}
		});

	// script tool is registered later
	_toolRegistry.registerTool(new voxedit::AnimationAddTool());
	_toolRegistry.registerTool(new voxedit::AnimationSetTool());
	_toolRegistry.registerTool(new voxedit::FindColorTool());
	_toolRegistry.registerTool(new voxedit::GetPaletteTool());
	_toolRegistry.registerTool(new voxedit::GetSceneStateTool());
	_toolRegistry.registerTool(new voxedit::GetVoxelsTool());
	_toolRegistry.registerTool(new voxedit::HistogramTool());
	_toolRegistry.registerTool(new voxedit::MementoCanRedoTool());
	_toolRegistry.registerTool(new voxedit::MementoCanUndoTool());
	_toolRegistry.registerTool(new voxedit::MementoRedoTool());
	_toolRegistry.registerTool(new voxedit::MementoUndoTool());
	_toolRegistry.registerTool(new voxedit::NodeAddKeyframeTool());
	_toolRegistry.registerTool(new voxedit::NodeAddModelTool());
	_toolRegistry.registerTool(new voxedit::NodeMoveTool());
	_toolRegistry.registerTool(new voxedit::NodeRemoveTool());
	_toolRegistry.registerTool(new voxedit::NodeRenameTool());
	_toolRegistry.registerTool(new voxedit::NodeSetPropertiesTool());
	_toolRegistry.registerTool(new voxedit::PlaceVoxelsTool());
	_toolRegistry.registerTool(new voxedit::ScriptApiTool());
	_toolRegistry.registerTool(new voxedit::ScriptCreateTool());
	_toolRegistry.registerTool(new voxedit::ShapeBrushTool());
	_toolRegistry.registerTool(new voxedit::PaintBrushTool());
	_toolRegistry.registerTool(new voxedit::LineBrushTool());
	_toolRegistry.registerTool(new voxedit::SelectBrushTool());
	_toolRegistry.registerTool(new voxedit::PlaneBrushTool());
	_toolRegistry.registerTool(new voxedit::CommandTool());
	_toolRegistry.registerTool(new voxedit::CommandListTool());

	return state;
}

void McpServer::updateScriptTools(const core::DynamicArray<voxedit::LuaScriptInfo> &scripts) {
	Log::debug("Received %d lua scripts from server", (int)scripts.size());
	for (const voxedit::LuaScriptInfo &script : scripts) {
		_toolRegistry.unregisterTool(voxedit::ScriptTool::toolName(script));
		_toolRegistry.registerTool(new voxedit::ScriptTool(script));
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

	const core::String host = core::getVar(cfg::VoxEditNetHostname)->strVal();
	const int port = core::getVar(cfg::VoxEditNetPort)->intVal();

	core::getVar(cfg::AppUserName)->setVal("mcp-client");
	core::getVar(cfg::AppVersion)->setVal(PROJECT_VERSION);

	if (!_sceneMgr->connectToServer(host, port)) {
		Log::error("Failed to connect to %s:%i", host.c_str(), port);
		return false;
	}

	requestScripts();
	return true;
}

void McpServer::disconnectFromVoxEdit() {
	_sceneMgr->client().disconnect();
}

bool McpServer::requestScripts() {
	voxedit::LuaScriptsRequestMessage requestMsg;
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
	if (!json::Json::accept(line)) {
		sendError(json::Json(), PARSE_ERROR, "Parse error");
		return true;
	}

	json::Json request = json::Json::parse(line);
	if (!request.isValid()) {
		sendError(json::Json(), PARSE_ERROR, "Parse error");
		return true;
	}

	handleRequest(request);
	return true;
}

void McpServer::handleRequest(const json::Json &request) {
	json::Json id = request.get("id");
	if (!request.contains("jsonrpc") || request.get("jsonrpc").str() != "2.0") {
		sendError(id, INVALID_REQUEST, "Invalid JSON-RPC version");
		return;
	}

	if (!request.contains("method") || !request.get("method").isString()) {
		sendError(id, INVALID_REQUEST, "Missing method");
		return;
	}

	const core::String method = request.get("method").str();
	Log::debug("Received MCP request for method %s", method.c_str());

	if (method == "initialize") {
		handleInitialize(request);
	} else if (method == "notifications/initialized") {
		Log::info("MCP client initialized");
	} else if (method == "tools/list") {
		if (!connectToVoxEdit()) {
			Log::error("Failed to connect to VoxEdit server at %s:%d",
					   core::getVar(cfg::VoxEditNetHostname)->strVal().c_str(),
					   core::getVar(cfg::VoxEditNetPort)->intVal());
			sendError(id, INIT_FAILED, "Failed to connect to VoxEdit server");
			return;
		}
		handleToolsList(request);
	} else if (method == "tools/call") {
		handleToolsCall(request);
	} else if (method == "ping") {
		json::Json response = json::Json::object();
		response.set("jsonrpc", "2.0");
		response.set("id", id);
		response.set("result", json::Json::object());
		sendResponse(response);
	} else {
		sendError(id, METHOD_NOT_FOUND, "Method not found");
	}
}

void McpServer::handleInitialize(const json::Json &request) {
	Log::info("Received initialize request");
	if (request.contains("params") && request.get("params").contains("clientInfo")) {
		const json::Json clientInfo = request.get("params").get("clientInfo");
		const core::String clientName = clientInfo.strVal("name", "unknown");
		const core::String clientVersion = clientInfo.strVal("version", "unknown");
		Log::info("Client: %s (version %s)", clientName.c_str(), clientVersion.c_str());
	}
	_initialized = true;

	json::Json result = json::Json::object();
	result.set("protocolVersion", "2024-11-05");

	json::Json capabilities = json::Json::object();
	json::Json tools = json::Json::object();
	tools.set("listChanged", true);
	capabilities.set("tools", tools);
	result.set("capabilities", capabilities);

	json::Json serverInfo = json::Json::object();
	serverInfo.set("name", appname().c_str());
	serverInfo.set("version", PROJECT_VERSION);
	result.set("serverInfo", serverInfo);

	json::Json response = json::Json::object();
	response.set("jsonrpc", "2.0");
	response.set("id", request.get("id"));
	response.set("result", result);
	sendResponse(response);
}

void McpServer::handleToolsList(const json::Json &request) {
	Log::info("Received tools list request");
	json::Json toolsArr = json::Json::array();
	_toolRegistry.addRegisteredTools(toolsArr);
	json::Json result = json::Json::object();
	result.set("tools", toolsArr);

	json::Json response = json::Json::object();
	response.set("jsonrpc", "2.0");
	response.set("id", request.get("id"));
	response.set("result", result);
	sendResponse(response);
}

void McpServer::handleToolsCall(const json::Json &request) {
	if (!request.contains("params") || !request.get("params").contains("name")) {
		sendError(request.get("id"), INVALID_PARAMS, "Missing tool name");
		return;
	}

	const core::String toolName = request.get("params").get("name").str();
	json::Json args = request.get("params").get("arguments");
	if (!args.isValid()) {
		args = json::Json::object();
	}
	const json::Json id = request.get("id");

	Log::info("Received tool call for %s", toolName.c_str());

	voxedit::ToolContext ctx;
	ctx.sceneMgr = _sceneMgr.get();
	ctx.result = sendToolResult;
	if (!_toolRegistry.call(toolName, id, args, ctx)) {
		sendError(id, INVALID_PARAMS, "Unknown tool");
	}
}

bool McpServer::sendToolResult(const json::Json &id, const core::String &text, bool isError) {
	if (isError) {
		Log::warn("Tool result error: %s", text.c_str());
	}
	json::Json result = json::Json::object();
	json::Json contentArr = json::Json::array();
	json::Json content = json::Json::object();
	content.set("type", "text");
	content.set("text", text.c_str());
	contentArr.push(content);
	result.set("content", contentArr);
	if (isError) {
		result.set("isError", true);
	}

	json::Json response = json::Json::object();
	response.set("jsonrpc", "2.0");
	response.set("id", id);
	response.set("result", result);
	sendResponse(response);
	return !isError;
}

void McpServer::sendResponse(const json::Json &response) {
	const core::String out = response.dump();
	Log::debug("Sending MCP response: %s", out.c_str());
	core_assert(out.find("\n") == core::String::npos);
	fprintf(stdout, "%s\n", out.c_str());
	fflush(stdout);
}

void McpServer::sendError(const json::Json &id, int code, const core::String &message) {
	Log::warn("Sending error %d: %s", code, message.c_str());
	json::Json response = json::Json::object();
	response.set("jsonrpc", "2.0");
	response.set("id", id);
	json::Json error = json::Json::object();
	error.set("code", code);
	error.set("message", message.c_str());
	response.set("error", error);
	sendResponse(response);
}

CONSOLE_APP(McpServer)
