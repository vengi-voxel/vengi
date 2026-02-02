/**
 * @file
 */

#include "McpServer.h"
#include "command/Command.h"
#include "commonlua/LUA.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "engine-config.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Filesystem.h"
#include "palette/FormatConfig.h"
#include "scenegraph/JsonExporter.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/network/Client.h"
#include "voxedit-util/network/ClientNetwork.h"
#include "voxedit-util/network/ProtocolIds.h"
#include "voxedit-util/network/protocol/CommandMessage.h"
#include "voxedit-util/network/protocol/CommandsRequestMessage.h"
#include "voxedit-util/network/protocol/LuaScriptCreateMessage.h"
#include "voxedit-util/network/protocol/LuaScriptsRequestMessage.h"
#include "voxedit-util/network/protocol/VoxelModificationMessage.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/FormatConfig.h"
#include "voxelgenerator/LUAApi.h"
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define STDIN_FILENO 0
#else
#include <sys/select.h>
#include <unistd.h>
#endif
#include <stdio.h>

// JSON-RPC error codes
static constexpr int PARSE_ERROR = -32700;
static constexpr int INVALID_REQUEST = -32600;
static constexpr int METHOD_NOT_FOUND = -32601;
static constexpr int INVALID_PARAMS = -32602;
static constexpr int INIT_FAILED = -32000;

void LuaScriptsListHandler::execute(const network::ClientId &clientId, voxedit::LuaScriptsListMessage *message) {
	_server->_scripts = message->scripts();
	_server->_scriptsReceived = true;
	Log::debug("Received %d lua scripts from server", (int)_server->_scripts.size());
}

void CommandsListHandler::execute(const network::ClientId &clientId, voxedit::CommandsListMessage *message) {
	_server->_commands = message->commands();
	_server->_commandsReceived = true;
	Log::debug("Received %d commands from server", (int)_server->_commands.size());
}

McpServer::McpServer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider, 1), _sceneRenderer(core::make_shared<voxedit::ISceneRenderer>()),
	  _modifierRenderer(core::make_shared<voxedit::IModifierRenderer>()),
	  _sceneMgr(core::make_shared<voxedit::SceneManager>(timeProvider, filesystem, _sceneRenderer, _modifierRenderer)),
	  _luaScriptsListHandler(this), _commandsListHandler(this) {
	init(ORGANISATION, "vengimcp");
}

app::AppState McpServer::onConstruct() {
	const app::AppState state = Super::onConstruct();
	core::Var::get(cfg::UILastDirectory, filesystem()->homePath().c_str());
	core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");
	core::Var::get(cfg::ClientCameraZoomSpeed, "0.1");
	_sceneMgr->construct();
	return state;
}

app::AppState McpServer::onInit() {
	const app::AppState state = Super::onInit();

	voxelformat::FormatConfig::init();
	palette::FormatConfig::init();

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

bool McpServer::sendCommand(const core::String &command) {
	voxedit::CommandMessage msg(command, core::Var::getSafe(cfg::VoxEditNetRconPassword)->strVal());
	return _sceneMgr->client().network().sendMessage(msg);
}

bool McpServer::createLuaScript(const core::String &name, const core::String &content) {
	voxedit::LuaScriptCreateMessage msg(name, content, core::Var::getSafe(cfg::VoxEditNetRconPassword)->strVal());
	return _sceneMgr->client().network().sendMessage(msg);
}

bool McpServer::requestScripts() {
	_scriptsReceived = false;
	voxedit::LuaScriptsRequestMessage requestMsg;
	return _sceneMgr->client().network().sendMessage(requestMsg);
}

bool McpServer::requestCommands() {
	_commandsReceived = false;
	voxedit::CommandsRequestMessage requestMsg;
	return _sceneMgr->client().network().sendMessage(requestMsg);
}

bool McpServer::sendVoxelModification(const core::UUID &nodeUUID, const voxel::RawVolume &volume,
									  const voxel::Region &region) {
	voxedit::VoxelModificationMessage msg(nodeUUID, volume, region);
	return _sceneMgr->client().network().sendMessage(msg);
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
			// Reset state
			_scriptsReceived = false;
			_commandsReceived = false;
			_scripts.clear();
			_commands.clear();

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

	// Use select to check if stdin has data available (non-blocking)
#ifdef _WIN32
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (WaitForSingleObject(hStdin, 100) != WAIT_OBJECT_0) {
		// No stdin data available, continue processing network messages
		return app::AppState::Running;
	}
#else
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000; // 100ms timeout

	const int stdinReady = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
	if (stdinReady <= 0 || !FD_ISSET(STDIN_FILENO, &readfds)) {
		// No stdin data available, continue processing network messages
		return app::AppState::Running;
	}
#endif

	Log::debug("Reading MCP request from stdin...");

	char line[65536];
	if (fgets(line, sizeof(line), stdin) == nullptr) {
		Log::error("Failed to read from stdin");
		return app::AppState::Cleanup;
	}

	if (line[0] == '\n' || line[0] == '\0') {
		Log::debug("Received empty MCP request");
		return app::AppState::Running;
	}

	if (!nlohmann::json::accept(line)) {
		sendError(nullptr, PARSE_ERROR, "Parse error");
		return app::AppState::Running;
	}

	nlohmann::json request = nlohmann::json::parse(line);
	if (request.is_discarded()) {
		sendError(nullptr, PARSE_ERROR, "Parse error");
		return app::AppState::Running;
	}

	handleRequest(request);
	return app::AppState::Running;
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
		if (!connectToVoxEdit()) {
			Log::error("Failed to connect to VoxEdit server at %s:%d",
					   core::Var::getSafe(cfg::VoxEditNetHostname)->strVal().c_str(),
					   core::Var::getSafe(cfg::VoxEditNetPort)->intVal());
			sendError(id, INIT_FAILED, "Failed to connect to VoxEdit server");
			return;
		}
		Log::info("MCP client initialized");
	} else if (method == "tools/list") {
		handleToolsList(request);
	} else if (method == "tools/call") {
		handleToolsCall(request);
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

void McpServer::scriptTools(nlohmann::json &tools) {
	for (const voxedit::LuaScriptInfo &script : _scripts) {
		core::String name = script.filename;
		if (name.size() > 4 && name.substr(name.size() - 4) == ".lua") {
			name = name.substr(0, name.size() - 4);
		}
		nlohmann::json tool;
		tool["name"] = core::String("voxedit_script_" + name).c_str();
		if (script.description.empty()) {
			tool["description"] = script.filename.c_str();
		} else {
			tool["description"] = script.description.c_str();
		}

		nlohmann::json inputSchema;
		inputSchema["type"] = "object";
		nlohmann::json properties = nlohmann::json::object();
		nlohmann::json required = nlohmann::json::array();

		for (const voxedit::LuaParameterInfo &param : script.parameters) {
			nlohmann::json propSchema;
			switch (param.type) {
			case voxedit::LuaParameterType::Integer:
			case voxedit::LuaParameterType::ColorIndex:
				propSchema["type"] = "integer";
				if (param.minValue <= param.maxValue) {
					propSchema["minimum"] = (int)param.minValue;
					propSchema["maximum"] = (int)param.maxValue;
				}
				break;
			case voxedit::LuaParameterType::Float:
				propSchema["type"] = "number";
				if (param.minValue <= param.maxValue) {
					propSchema["minimum"] = param.minValue;
					propSchema["maximum"] = param.maxValue;
				}
				break;
			case voxedit::LuaParameterType::Boolean:
				propSchema["type"] = "boolean";
				break;
			case voxedit::LuaParameterType::Enum:
				propSchema["type"] = "string";
				if (!param.enumValues.empty()) {
					nlohmann::json enumArray = nlohmann::json::array();
					core::DynamicArray<core::String> values;
					core::string::splitString(param.enumValues, values, ";");
					for (const core::String &v : values) {
						enumArray.push_back(v.c_str());
					}
					propSchema["enum"] = core::move(enumArray);
				}
				break;
			case voxedit::LuaParameterType::String:
			case voxedit::LuaParameterType::File:
			default:
				propSchema["type"] = "string";
				break;
			}
			if (!param.description.empty()) {
				propSchema["description"] = param.description.c_str();
			} else {
				propSchema["description"] = param.name.c_str();
			}
			if (!param.defaultValue.empty()) {
				if (param.type == voxedit::LuaParameterType::Integer ||
					param.type == voxedit::LuaParameterType::ColorIndex) {
					propSchema["default"] = core::string::toInt(param.defaultValue);
				} else if (param.type == voxedit::LuaParameterType::Float) {
					propSchema["default"] = core::string::toFloat(param.defaultValue);
				} else if (param.type == voxedit::LuaParameterType::Boolean) {
					propSchema["default"] = core::string::toBool(param.defaultValue);
				} else {
					propSchema["default"] = param.defaultValue.c_str();
				}
			}
			properties[param.name.c_str()] = core::move(propSchema);
			// All script parameters are required unless they have a default value
			if (param.defaultValue.empty()) {
				required.push_back(param.name.c_str());
			}
		}

		inputSchema["properties"] = core::move(properties);
		if (!required.empty()) {
			inputSchema["required"] = core::move(required);
		}
		tool["inputSchema"] = core::move(inputSchema);
		tools.emplace_back(core::move(tool));
	}
}

void McpServer::commandTools(nlohmann::json &tools) {
	for (const voxedit::CommandInfo &cmd : _commands) {
		nlohmann::json tool;
		core::String toolName;
		if (cmd.name[0] == COMMAND_PRESSED[0]) {
			toolName = core::String::format("voxedit_cmd_pressed_%s", cmd.name.c_str() + 1);
			const core::String desc = core::String::format(
				"Execute input command '%s' (pressed - make sure to call the release version afterwards)",
				cmd.name.c_str() + 1);
			tool["description"] = desc.c_str();
		} else if (cmd.name[0] == COMMAND_RELEASED[0]) {
			toolName = core::String::format("voxedit_cmd_released_%s", cmd.name.c_str() + 1);
			const core::String desc = core::String::format(
				"Execute input command '%s' (released - make sure to call the pressed version beforehand)",
				cmd.name.c_str() + 1);
			tool["description"] = desc.c_str();
		} else {
			toolName = core::String("voxedit_cmd_" + cmd.name);
			tool["description"] = cmd.description.c_str();
		}
		tool["name"] = toolName.c_str();

		nlohmann::json argsProp;
		argsProp["type"] = "string";
		argsProp["description"] = "Command arguments";

		nlohmann::json inputSchema;
		inputSchema["type"] = "object";
		inputSchema["properties"]["args"] = core::move(argsProp);
		tool["inputSchema"] = core::move(inputSchema);
		tools.emplace_back(core::move(tool));
	}
}

void McpServer::findColorTool(nlohmann::json &tools) {
	nlohmann::json tool;
	tool["name"] = "voxedit_find_color";
	tool["description"] = "Find the closest matching color index in a node's palette for a given RGBA color. "
						  "Returns the palette index to use with voxedit_place_voxels.";

	nlohmann::json nodeUUIDProp;
	nodeUUIDProp["type"] = "string";
	nodeUUIDProp["description"] = "UUID of the node whose palette to search";

	nlohmann::json rProp;
	rProp["type"] = "integer";
	rProp["description"] = "Red component (0-255)";
	rProp["minimum"] = 0;
	rProp["maximum"] = 255;

	nlohmann::json gProp;
	gProp["type"] = "integer";
	gProp["description"] = "Green component (0-255)";
	gProp["minimum"] = 0;
	gProp["maximum"] = 255;

	nlohmann::json bProp;
	bProp["type"] = "integer";
	bProp["description"] = "Blue component (0-255)";
	bProp["minimum"] = 0;
	bProp["maximum"] = 255;

	nlohmann::json aProp;
	aProp["type"] = "integer";
	aProp["description"] = "Alpha component (0-255), defaults to 255";
	aProp["minimum"] = 0;
	aProp["maximum"] = 255;
	aProp["default"] = 255;

	nlohmann::json properties = nlohmann::json::object();
	properties["nodeUUID"] = core::move(nodeUUIDProp);
	properties["r"] = core::move(rProp);
	properties["g"] = core::move(gProp);
	properties["b"] = core::move(bProp);
	properties["a"] = core::move(aProp);

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID", "r", "g", "b"});
	inputSchema["properties"] = core::move(properties);
	tool["inputSchema"] = core::move(inputSchema);
	tools.emplace_back(core::move(tool));
}

void McpServer::getPaletteTool(nlohmann::json &tools) {
	nlohmann::json tool;
	tool["name"] = "voxedit_get_palette";
	tool["description"] =
		"Get the color palette of a specific node. Returns all colors with their RGBA values, "
		"names, and material properties. Use this to find the right colorIndex for voxedit_place_voxels.";

	nlohmann::json nodeUUIDProp;
	nodeUUIDProp["type"] = "string";
	nodeUUIDProp["description"] = "UUID of the node to get the palette from";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"nodeUUID"});
	inputSchema["properties"]["nodeUUID"] = core::move(nodeUUIDProp);
	tool["inputSchema"] = core::move(inputSchema);
	tools.emplace_back(core::move(tool));
}

void McpServer::placeVoxelTool(nlohmann::json &tools) {
	nlohmann::json tool;
	tool["name"] = "voxedit_place_voxels";
	tool["description"] = "Place voxels at specified positions in a node. Get node UUIDs from voxedit_get_scene_state.";

	nlohmann::json itemsSchema;
	itemsSchema["type"] = "object";
	itemsSchema["properties"]["x"]["type"] = "integer";
	itemsSchema["properties"]["x"]["description"] = "X coordinate";
	itemsSchema["properties"]["y"]["type"] = "integer";
	itemsSchema["properties"]["y"]["description"] = "Y coordinate";
	itemsSchema["properties"]["z"]["type"] = "integer";
	itemsSchema["properties"]["z"]["description"] = "Z coordinate";
	itemsSchema["properties"]["colorIndex"]["type"] = "integer";
	itemsSchema["properties"]["colorIndex"]["description"] = "Palette index of the color";
	itemsSchema["required"] = nlohmann::json::array({"x", "y", "z", "colorIndex"});

	nlohmann::json voxelsProp;
	voxelsProp["type"] = "array";
	voxelsProp["description"] = "Array of {x, y, z, colorIndex} objects";
	voxelsProp["items"] = core::move(itemsSchema);

	nlohmann::json nodeUUIDProp;
	nodeUUIDProp["type"] = "string";
	nodeUUIDProp["description"] = "UUID of the node to modify";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"voxels", "nodeUUID"});
	inputSchema["properties"]["voxels"] = core::move(voxelsProp);
	inputSchema["properties"]["nodeUUID"] = core::move(nodeUUIDProp);
	tool["inputSchema"] = core::move(inputSchema);
	tools.emplace_back(core::move(tool));
}

void McpServer::getSceneStateTool(nlohmann::json &tools) {
	nlohmann::json tool;
	tool["name"] = "voxedit_get_scene_state";
	tool["description"] = "Get the current scene graph structure as JSON. Each node includes its UUID, type, "
						  "palette colors (with RGBA values and names), and volume information.";
	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["properties"] = nlohmann::json::object();
	tool["inputSchema"] = core::move(inputSchema);
	tools.emplace_back(core::move(tool));
}

void McpServer::createGeneratorTool(nlohmann::json &tools) {
	nlohmann::json tool;
	tool["name"] = "voxedit_create_generator";
	tool["description"] = "Create and run a custom Lua generator script.\n"
						  "Script receives: node, region, color, [custom args]\n"
						  "Get api details with voxedit_lua_api.\n"
						  "Always add a description function to your script.\n"
						  "function description() return 'My script description' end\n"
						  "Custom args are given via function arguments() return { { name = 'padding', desc = "
						  "'padding between nodes', type = 'int', default = '2' } } end\n"
						  "Try to use arguments to make your scripts re-usable.";

	nlohmann::json nameProp;
	nameProp["type"] = "string";
	nameProp["description"] = "Script name";

	nlohmann::json codeProp;
	codeProp["type"] = "string";
	codeProp["description"] = "Lua script code";

	nlohmann::json runProp;
	runProp["type"] = "boolean";
	runProp["default"] = true;
	runProp["description"] = "Run the script immediately";

	nlohmann::json argsProp;
	argsProp["type"] = "string";
	argsProp["description"] = "Script arguments defined by function arguments() in the lua code [custom args]";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["required"] = nlohmann::json::array({"name", "code"});
	inputSchema["properties"]["name"] = core::move(nameProp);
	inputSchema["properties"]["code"] = core::move(codeProp);
	inputSchema["properties"]["run"] = core::move(runProp);
	inputSchema["properties"]["args"] = core::move(argsProp);
	tool["inputSchema"] = core::move(inputSchema);
	tools.emplace_back(core::move(tool));
}

void McpServer::luaApiDocTool(nlohmann::json &tools) {
	nlohmann::json tool;
	tool["name"] = "voxedit_lua_api";
	tool["description"] = "Get the Lua API documentation as JSON. This describes all available Lua functions and "
						  "their parameters for writing generator scripts.";
	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["properties"] = nlohmann::json::object();
	tool["inputSchema"] = core::move(inputSchema);
	tools.emplace_back(core::move(tool));
}

void McpServer::handleToolsList(const nlohmann::json &request) {
	Log::info("Received tools list request");
	nlohmann::json tools = nlohmann::json::array();

	luaApiDocTool(tools);
	createGeneratorTool(tools);
	getSceneStateTool(tools);
	placeVoxelTool(tools);
	getPaletteTool(tools);
	findColorTool(tools);
	commandTools(tools);
	scriptTools(tools);
	// TODO: add a tool to change the scene graph transforms and animate a scene

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

	// voxedit_cmd_* tools
	if (core::string::startsWith(toolName.c_str(), "voxedit_cmd_")) {
		std::string cmdName = toolName.substr(12);
		if (core::string::startsWith(cmdName.c_str(), "pressed_")) {
			// Convert the _ to pressed command
			cmdName = cmdName.substr(7);
			cmdName[0] = COMMAND_PRESSED[0];
		} else if (core::string::startsWith(cmdName.c_str(), "released_")) {
			// Convert the _ to released command
			cmdName = cmdName.substr(7);
			cmdName[0] = COMMAND_RELEASED[0];
		}
		const std::string &cmdArgs = args.value("args", "");
		const core::String &cmd = cmdArgs.empty() ? core::String(cmdName.c_str())
												  : core::String::format("%s %s", cmdName.c_str(), cmdArgs.c_str());
		if (sendCommand(cmd)) {
			sendToolResult(id, core::String::format("Executed: %s", cmd.c_str()));
		} else {
			sendToolResult(id, "Failed to send command", true);
		}
		return;
	}

	// voxedit_script_* tools
	if (core::string::startsWith(toolName.c_str(), "voxedit_script_")) {
		const std::string &scriptName = toolName.substr(15);
		const std::string &scriptArgs = args.value("args", "");
		const core::String &cmd = core::String::format("xs %s %s", scriptName.c_str(), scriptArgs.c_str());
		if (sendCommand(cmd)) {
			sendToolResult(id, core::String::format("Executed script: %s", scriptName.c_str()));
		} else {
			sendToolResult(id, "Failed to execute script", true);
		}
		return;
	}

	if (core::string::startsWith(toolName.c_str(), "voxedit_create_generator")) {
		const core::String name = args.value("name", "").c_str();
		const core::String code = args.value("code", "").c_str();
		const bool run = args.value("run", true);
		const core::String scriptArgs = args.value("args", "").c_str();

		voxelgenerator::LUAApi &luaApi = _sceneMgr->luaApi();
		voxelgenerator::LUAScript script;
		if (!luaApi.reloadScriptParameters(script, code)) {
			sendToolResult(id, "Failed to create script: " + luaApi.error(), true);
			return;
		}
		if (!script.valid) {
			sendToolResult(id, "Failed to create script: detected as invalid", true);
			return;
		}
		if (script.desc.empty()) {
			sendToolResult(id, "Failed to create script: missing description function", true);
			return;
		}

		if (!createLuaScript(name.c_str(), code.c_str())) {
			sendToolResult(id, "Failed to create script", true);
			return;
		}

		core::String result = core::String::format("Created script: %s", name.c_str());
		if (run) {
			const core::String &cmd = core::String::format("xs %s %s", name.c_str(), scriptArgs.c_str());
			if (sendCommand(cmd)) {
				result += " (executed)";
			}
		}
		sendToolResult(id, result);
		return;
	}

	if (core::string::startsWith(toolName.c_str(), "voxedit_lua_api")) {
		io::BufferedReadWriteStream stream;
		voxelgenerator::LUAApi &luaApi = _sceneMgr->luaApi();
		if (luaApi.apiJsonToStream(stream)) {
			core::String json((const char *)stream.getBuffer(), (size_t)stream.size());
			sendToolResult(id, json);
		} else {
			sendToolResult(id, "Failed to generate Lua API documentation", true);
		}
		return;
	}

	if (core::string::startsWith(toolName.c_str(), "voxedit_get_scene_state")) {
		if (_sceneMgr->sceneGraph().empty()) {
			sendToolResult(id, "Scene graph is empty - not connected or no scene loaded", true);
			return;
		}
		io::BufferedReadWriteStream stream;
		scenegraph::sceneGraphJson(_sceneMgr->sceneGraph(), false, stream);
		const core::String json((const char *)stream.getBuffer(), (size_t)stream.size());
		sendToolResult(id, json);
		return;
	}

	if (core::string::startsWith(toolName.c_str(), "voxedit_place_voxels")) {
		if (!args.contains("voxels")) {
			sendToolResult(id, "Missing voxels argument", true);
			return;
		}
		if (!args.contains("nodeUUID") || !args["nodeUUID"].is_string()) {
			sendToolResult(id, "Missing nodeUUID argument", true);
			return;
		}

		const core::UUID nodeUUID(args["nodeUUID"].get<std::string>().c_str());
		if (!nodeUUID.isValid()) {
			sendToolResult(id, "Invalid node UUID", true);
			return;
		}

		const nlohmann::json &voxelsArray = args["voxels"];
		if (!voxelsArray.is_array() || voxelsArray.empty()) {
			sendToolResult(id, "voxels must be a non-empty array", true);
			return;
		}

		voxel::SparseVolume volume;
		int placedCount = 0;
		for (const auto &voxelData : voxelsArray) {
			const int x = voxelData["x"].get<int>();
			const int y = voxelData["y"].get<int>();
			const int z = voxelData["z"].get<int>();
			const int colorIndex = voxelData.value("colorIndex", 1);
			if (colorIndex > 0 && colorIndex < 256) {
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, colorIndex));
				++placedCount;
			}
		}

		voxel::RawVolume rawVolume(volume.calculateRegion());
		volume.copyTo(rawVolume);
		if (sendVoxelModification(nodeUUID, rawVolume, rawVolume.region())) {
			sendToolResult(id,
						   core::String::format("Placed %d voxels in node %s", placedCount, nodeUUID.str().c_str()));
		} else {
			sendToolResult(id, "Failed to send voxel modification", true);
		}
		return;
	}

	if (core::string::startsWith(toolName.c_str(), "voxedit_get_palette")) {
		if (!args.contains("nodeUUID") || !args["nodeUUID"].is_string()) {
			sendToolResult(id, "Missing nodeUUID argument", true);
			return;
		}

		const core::UUID nodeUUID(args["nodeUUID"].get<std::string>().c_str());
		if (!nodeUUID.isValid()) {
			sendToolResult(id, "Invalid node UUID", true);
			return;
		}

		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
		if (node == nullptr) {
			sendToolResult(id, "Node not found", true);
			return;
		}

		const palette::Palette &palette = node->palette();
		nlohmann::json paletteJson;
		paletteJson["name"] = palette.name().c_str();
		paletteJson["colorCount"] = palette.colorCount();
		paletteJson["colors"] = nlohmann::json::array();
		for (size_t i = 0; i < palette.size(); ++i) {
			const color::RGBA &color = palette.color(i);
			nlohmann::json colorJson;
			colorJson["index"] = i;
			colorJson["r"] = color.r;
			colorJson["g"] = color.g;
			colorJson["b"] = color.b;
			colorJson["a"] = color.a;
			if (!palette.colorName(i).empty()) {
				colorJson["name"] = palette.colorName(i).c_str();
			}
			paletteJson["colors"].emplace_back(core::move(colorJson));
		}
		sendToolResult(id, paletteJson.dump().c_str());
		return;
	}

	if (core::string::startsWith(toolName.c_str(), "voxedit_find_color")) {
		if (!args.contains("nodeUUID") || !args["nodeUUID"].is_string()) {
			sendToolResult(id, "Missing nodeUUID argument", true);
			return;
		}
		if (!args.contains("r") || !args.contains("g") || !args.contains("b")) {
			sendToolResult(id, "Missing r, g, or b argument", true);
			return;
		}

		const core::UUID nodeUUID(args["nodeUUID"].get<std::string>().c_str());
		if (!nodeUUID.isValid()) {
			sendToolResult(id, "Invalid node UUID", true);
			return;
		}

		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
		if (node == nullptr) {
			sendToolResult(id, "Node not found", true);
			return;
		}

		const int r = args["r"].get<int>();
		const int g = args["g"].get<int>();
		const int b = args["b"].get<int>();
		const int a = args.value("a", 255);
		const color::RGBA rgba((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);

		const palette::Palette &palette = node->palette();
		const int matchIndex = palette.getClosestMatch(rgba);

		nlohmann::json resultJson;
		resultJson["colorIndex"] = matchIndex;
		if (matchIndex >= 0 && matchIndex < (int)palette.size()) {
			const color::RGBA &matchedColor = palette.color(matchIndex);
			resultJson["matchedColor"]["r"] = matchedColor.r;
			resultJson["matchedColor"]["g"] = matchedColor.g;
			resultJson["matchedColor"]["b"] = matchedColor.b;
			resultJson["matchedColor"]["a"] = matchedColor.a;
			if (!palette.colorName(matchIndex).empty()) {
				resultJson["matchedColor"]["name"] = palette.colorName(matchIndex).c_str();
			}
		}
		sendToolResult(id, resultJson.dump().c_str());
		return;
	}

	sendError(id, INVALID_PARAMS, "Unknown tool");
}

void McpServer::sendToolResult(const nlohmann::json &id, const core::String &text, bool isError) {
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
