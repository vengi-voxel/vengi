/**
 * @file
 */

#include "McpServer.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "core/collection/Array.h"
#include "engine-config.h"
#include "io/BufferedReadWriteStream.h"
#include "io/File.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "network/NetworkError.h"
#include "network/NetworkImpl.h"
#include "palette/FormatConfig.h"
#include "scenegraph/JsonExporter.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/network/ProtocolIds.h"
#include "voxedit-util/network/ProtocolMessageFactory.h"
#include "voxedit-util/network/ProtocolVersion.h"
#include "voxedit-util/network/protocol/CommandMessage.h"
#include "voxedit-util/network/protocol/CommandsRequestMessage.h"
#include "voxedit-util/network/protocol/InitSessionMessage.h"
#include "voxedit-util/network/protocol/LuaScriptCreateMessage.h"
#include "voxedit-util/network/protocol/LuaScriptsRequestMessage.h"
#include "voxedit-util/network/protocol/SceneStateRequestMessage.h"
#include "voxedit-util/network/protocol/VoxelModificationMessage.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/FormatConfig.h"

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

void SceneStateHandler::execute(const network::ClientId &clientId, voxedit::SceneStateMessage *message) {
	_server->_sceneGraph = core::move(message->sceneGraph());
	_server->_sceneStateReceived = true;
	Log::debug("Received scene state with %d nodes", (int)_server->_sceneGraph.size());
}

McpServer::McpServer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider)
	: Super(filesystem, timeProvider, 1), _network(new network::NetworkImpl()), _luaScriptsListHandler(this),
	  _commandsListHandler(this), _sceneStateHandler(this), _luaApi(filesystem) {
	init(ORGANISATION, "vengimcp");
}

app::AppState McpServer::onConstruct() {
	_luaApi.construct();

	core::Var::get(cfg::VoxEditNetHostname, "127.0.0.1", "The hostname of the voxedit server");
	core::Var::get(cfg::VoxEditNetPort, "10001", "The port to run the voxedit server on");
	core::Var::get(cfg::VoxEditNetPassword, "", core::CV_SECRET,
				   "The password required to connect to the voxedit server");
	core::Var::get(cfg::VoxEditNetRconPassword, "changeme", core::CV_SECRET,
				   "The rcon password required to send commands to the voxedit server");

	return Super::onConstruct();
}

app::AppState McpServer::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	voxelformat::FormatConfig::init();
	palette::FormatConfig::init();

	if (!_luaApi.init()) {
		Log::error("Failed to initialize the LUA API");
		return app::AppState::InitFailure;
	}

	return app::AppState::Running;
}

app::AppState McpServer::onCleanup() {
	disconnectFromVoxEdit();
	_protocolRegistry.shutdown();
	delete _network;
	_luaApi.shutdown();
	return Super::onCleanup();
}

bool McpServer::connectToVoxEdit() {
#ifdef WIN32
	WSADATA wsaData;
	const int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaResult != NO_ERROR) {
		Log::error("WSAStartup failed with error: %d", wsaResult);
		return false;
	}
#else
	signal(SIGPIPE, SIG_IGN);
#endif

	FD_ZERO(&_network->readFDSet);
	FD_ZERO(&_network->writeFDSet);

	struct addrinfo hints, *res = nullptr;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	const core::String host = core::Var::getSafe(cfg::VoxEditNetHostname)->strVal();
	const int port = core::Var::getSafe(cfg::VoxEditNetPort)->intVal();
	const core::String service = core::string::toString(port);
	int err = getaddrinfo(host.c_str(), service.c_str(), &hints, &res);
	if (err != 0 || res == nullptr) {
		Log::error("Failed to resolve hostname %s: %s", host.c_str(), network::getNetworkErrorString());
		return false;
	}

	_network->socketFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (_network->socketFD == network::InvalidSocketId) {
		freeaddrinfo(res);
		Log::error("Failed to create socket: %s", network::getNetworkErrorString());
		return false;
	}

	int connectResult = ::connect(_network->socketFD, res->ai_addr, res->ai_addrlen);
	if (connectResult < 0) {
		closesocket(_network->socketFD);
		_network->socketFD = network::InvalidSocketId;
		freeaddrinfo(res);
		Log::error("Failed to connect to %s:%i: %s", host.c_str(), port, network::getNetworkErrorString());
		return false;
	}

	freeaddrinfo(res);
	FD_SET(_network->socketFD, &_network->readFDSet);

	// Register protocol handlers
	network::ProtocolHandlerRegistry &r = _protocolRegistry;
	r.registerHandler(voxedit::PROTO_PING, &_nopHandler);
	r.registerHandler(voxedit::PROTO_LUA_SCRIPTS_LIST, &_luaScriptsListHandler);
	r.registerHandler(voxedit::PROTO_COMMANDS_LIST, &_commandsListHandler);
	r.registerHandler(voxedit::PROTO_SCENE_STATE, &_sceneStateHandler);

	core::Var::getSafe(cfg::AppUserName)->setVal("mcp-client");

	voxedit::InitSessionMessage initMsg(false);
	return sendMessage(initMsg);
}

void McpServer::disconnectFromVoxEdit() {
	if (_network->socketFD != network::InvalidSocketId) {
		closesocket(_network->socketFD);
		_network->socketFD = network::InvalidSocketId;
	}
	FD_ZERO(&_network->readFDSet);
	FD_ZERO(&_network->writeFDSet);
	network_cleanup();
}

bool McpServer::sendMessage(const network::ProtocolMessage &msg) {
	if (_network->socketFD == network::InvalidSocketId) {
		return false;
	}

	const size_t total = msg.size();
	Log::debug("Send message of type %d with size %u to server", msg.getId(), (uint32_t)total);
	size_t sentTotal = 0;
	while (sentTotal < total) {
		const size_t toSend = total - sentTotal;
		const network_return sent = send(_network->socketFD, (const char *)msg.getBuffer() + sentTotal, toSend, 0);
		if (sent < 0) {
			Log::warn("Failed to send message: %s", network::getNetworkErrorString());
			return false;
		}
		sentTotal += sent;
	}
	return true;
}

void McpServer::processIncomingMessages() {
	if (_network->socketFD == network::InvalidSocketId) {
		return;
	}

	fd_set readFDsOut = _network->readFDSet;

	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
#ifdef WIN32
	const int ready = select(0, &readFDsOut, nullptr, nullptr, &tv);
#else
	const int ready = select(_network->socketFD + 1, &readFDsOut, nullptr, nullptr, &tv);
#endif
	if (ready <= 0) {
		return;
	}

	if (!FD_ISSET(_network->socketFD, &readFDsOut)) {
		return;
	}

	core::Array<uint8_t, 16384> buf;
	const network_return len = recv(_network->socketFD, (char *)&buf[0], buf.size(), 0);
	if (len <= 0) {
		Log::debug("No data received from VoxEdit server (len=%d)", (int)len);
		return;
	}
	Log::debug("Received %d bytes from VoxEdit server", (int)len);
	_inStream.write(buf.data(), len);

	// Process all available messages using registered handlers
	while (voxedit::ProtocolMessageFactory::isNewMessageAvailable(_inStream)) {
		Log::debug("Processing message from stream (stream size: %d)", (int)_inStream.size());
		core::ScopedPtr<network::ProtocolMessage> msg(voxedit::ProtocolMessageFactory::create(_inStream));
		if (!msg) {
			Log::warn("Received invalid message");
			break;
		}
		Log::debug("Received message type %d", (int)msg->getId());
		if (network::ProtocolHandler *handler = _protocolRegistry.getHandler(*msg)) {
			handler->execute(0, *msg);
		} else {
			Log::debug("No handler for message type %d", (int)msg->getId());
		}
	}
}

bool McpServer::sendCommand(const core::String &command) {
	voxedit::CommandMessage msg(command, core::Var::getSafe(cfg::VoxEditNetRconPassword)->strVal());
	return sendMessage(msg);
}

bool McpServer::createLuaScript(const core::String &name, const core::String &content) {
	voxedit::LuaScriptCreateMessage msg(name, content, core::Var::getSafe(cfg::VoxEditNetRconPassword)->strVal());
	return sendMessage(msg);
}

bool McpServer::requestScripts() {
	_scriptsReceived = false;
	voxedit::LuaScriptsRequestMessage requestMsg;
	return sendMessage(requestMsg);
}

bool McpServer::requestCommands() {
	_commandsReceived = false;
	voxedit::CommandsRequestMessage requestMsg;
	return sendMessage(requestMsg);
}

bool McpServer::sendVoxelModification(const core::UUID &nodeUUID, const voxel::RawVolume &volume,
									  const voxel::Region &region) {
	voxedit::VoxelModificationMessage msg(nodeUUID, volume, region);
	return sendMessage(msg);
}

app::AppState McpServer::onRunning() {
	// Process any pending messages from the VoxEdit server
	processIncomingMessages();

	// Use select to check if stdin has data available (non-blocking)
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

	char line[65536];
	if (fgets(line, sizeof(line), stdin) == nullptr) {
		Log::error("Failed to read from stdin");
		return app::AppState::Cleanup;
	}

	if (line[0] == '\n' || line[0] == '\0') {
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
	if (!request.contains("jsonrpc") || request["jsonrpc"] != "2.0") {
		sendError(request.value("id", nlohmann::json()), INVALID_REQUEST, "Invalid JSON-RPC version");
		return;
	}

	if (!request.contains("method") || !request["method"].is_string()) {
		sendError(request.value("id", nlohmann::json()), INVALID_REQUEST, "Missing method");
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
			sendError(request.value("id", nlohmann::json()), INIT_FAILED, "Failed to connect to VoxEdit server");
			return;
		}

		requestScripts();
		requestCommands();

		Log::info("MCP client initialized");
	} else if (method == "tools/list") {
		handleToolsList(request);
	} else if (method == "tools/call") {
		handleToolsCall(request);
	} else {
		sendError(request.value("id", nlohmann::json()), METHOD_NOT_FOUND, "Method not found");
	}
}

void McpServer::handleInitialize(const nlohmann::json &request) {
	Log::info("Received initialize request");
	_initialized = true;

	nlohmann::json result;
	result["protocolVersion"] = "2024-11-05";
	result["capabilities"]["tools"]["listChanged"] = true;
	result["serverInfo"]["name"] = appname();
	result["serverInfo"]["version"] = PROJECT_VERSION;

	nlohmann::json response;
	response["jsonrpc"] = "2.0";
	response["id"] = request.value("id", nlohmann::json());
	response["result"] = result;
	sendResponse(response);
}

void McpServer::handleToolsList(const nlohmann::json &request) {
	Log::info("Received tools list request");
	nlohmann::json tools = nlohmann::json::array();

	// voxedit_lua_api - expose the Lua API documentation
	{
		nlohmann::json tool;
		tool["name"] = "voxedit_lua_api";
		tool["description"] = "Get the Lua API documentation as JSON. This describes all available Lua functions and "
							  "their parameters for writing generator scripts.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"] = nlohmann::json::object();
		tools.push_back(tool);
	}

	// voxedit_create_generator
	{
		nlohmann::json tool;
		tool["name"] = "voxedit_create_generator";
		tool["description"] = "Create and run a custom Lua generator script.\n"
							  "Script receives: node, region, color, [custom args]\n"
							  "Get api details with voxedit_lua_api";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["required"] = nlohmann::json::array({"name", "code"});
		tool["inputSchema"]["properties"]["name"]["type"] = "string";
		tool["inputSchema"]["properties"]["name"]["description"] = "Script name";
		tool["inputSchema"]["properties"]["code"]["type"] = "string";
		tool["inputSchema"]["properties"]["code"]["description"] = "Lua script code";
		tool["inputSchema"]["properties"]["run"]["type"] = "boolean";
		tool["inputSchema"]["properties"]["run"]["default"] = true;
		tool["inputSchema"]["properties"]["args"]["type"] = "string";
		tool["inputSchema"]["properties"]["args"]["description"] = "Script arguments";
		tools.push_back(tool);
	}

	// voxedit_get_scene_state
	{
		nlohmann::json tool;
		tool["name"] = "voxedit_get_scene_state";
		tool["description"] = "Get the current scene graph structure as JSON. Each node includes its UUID, type, "
							  "palette colors (with RGBA values and names), and volume information.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"] = nlohmann::json::object();
		tools.push_back(tool);
	}

	// voxedit_place_voxels
	{
		nlohmann::json tool;
		tool["name"] = "voxedit_place_voxels";
		tool["description"] =
			"Place voxels at specified positions in a node. Get node UUIDs from voxedit_get_scene_state.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["required"] = nlohmann::json::array({"voxels", "nodeUUID"});
		tool["inputSchema"]["properties"]["voxels"]["type"] = "array";
		tool["inputSchema"]["properties"]["voxels"]["description"] = "Array of {x, y, z, colorIndex} objects";
		tool["inputSchema"]["properties"]["nodeUUID"]["type"] = "string";
		tool["inputSchema"]["properties"]["nodeUUID"]["description"] = "UUID of the node to modify";
		tools.push_back(tool);
	}

	// voxedit_get_palette
	{
		nlohmann::json tool;
		tool["name"] = "voxedit_get_palette";
		tool["description"] = "Get the color palette of a specific node. Returns all colors with their RGBA values, "
							  "names, and material properties. Use this to find the right colorIndex for voxedit_place_voxels.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["required"] = nlohmann::json::array({"nodeUUID"});
		tool["inputSchema"]["properties"]["nodeUUID"]["type"] = "string";
		tool["inputSchema"]["properties"]["nodeUUID"]["description"] = "UUID of the node to get the palette from";
		tools.push_back(tool);
	}

	// voxedit_find_color
	{
		nlohmann::json tool;
		tool["name"] = "voxedit_find_color";
		tool["description"] = "Find the closest matching color index in a node's palette for a given RGBA color. "
							  "Returns the palette index to use with voxedit_place_voxels.";
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["required"] = nlohmann::json::array({"nodeUUID", "r", "g", "b"});
		tool["inputSchema"]["properties"]["nodeUUID"]["type"] = "string";
		tool["inputSchema"]["properties"]["nodeUUID"]["description"] = "UUID of the node whose palette to search";
		tool["inputSchema"]["properties"]["r"]["type"] = "integer";
		tool["inputSchema"]["properties"]["r"]["description"] = "Red component (0-255)";
		tool["inputSchema"]["properties"]["r"]["minimum"] = 0;
		tool["inputSchema"]["properties"]["r"]["maximum"] = 255;
		tool["inputSchema"]["properties"]["g"]["type"] = "integer";
		tool["inputSchema"]["properties"]["g"]["description"] = "Green component (0-255)";
		tool["inputSchema"]["properties"]["g"]["minimum"] = 0;
		tool["inputSchema"]["properties"]["g"]["maximum"] = 255;
		tool["inputSchema"]["properties"]["b"]["type"] = "integer";
		tool["inputSchema"]["properties"]["b"]["description"] = "Blue component (0-255)";
		tool["inputSchema"]["properties"]["b"]["minimum"] = 0;
		tool["inputSchema"]["properties"]["b"]["maximum"] = 255;
		tool["inputSchema"]["properties"]["a"]["type"] = "integer";
		tool["inputSchema"]["properties"]["a"]["description"] = "Alpha component (0-255), defaults to 255";
		tool["inputSchema"]["properties"]["a"]["minimum"] = 0;
		tool["inputSchema"]["properties"]["a"]["maximum"] = 255;
		tool["inputSchema"]["properties"]["a"]["default"] = 255;
		tools.push_back(tool);
	}

	// Dynamic command tools
	for (const voxedit::CommandInfo &cmd : _commands) {
		nlohmann::json tool;
		tool["name"] = core::String("voxedit_cmd_" + cmd.name).c_str();
		tool["description"] = cmd.description.c_str();
		tool["inputSchema"]["type"] = "object";
		tool["inputSchema"]["properties"]["args"]["type"] = "string";
		tool["inputSchema"]["properties"]["args"]["description"] = "Command arguments";
		tools.push_back(tool);
	}

	// TODO: add a tool to change the scene graph transforms and animate a scene

	// Dynamic script tools
	for (const voxedit::LuaScriptInfo &script : _scripts) {
		core::String name = script.filename;
		if (name.size() > 4 && name.substr(name.size() - 4) == ".lua") {
			name = name.substr(0, name.size() - 4);
		}
		nlohmann::json tool;
		tool["name"] = core::String("voxedit_script_" + name).c_str();
		if (script.description.empty()) {
			tool["description"] = script.filename;
		} else {
			tool["description"] = script.description;
		}
		tool["inputSchema"]["type"] = "object";
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
					propSchema["enum"] = enumArray;
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
			tool["inputSchema"]["properties"][param.name.c_str()] = propSchema;
			// All script parameters are required unless they have a default value
			if (param.defaultValue.empty()) {
				required.push_back(param.name.c_str());
			}
		}
		if (!required.empty()) {
			tool["inputSchema"]["required"] = required;
		}
		tools.push_back(tool);
	}

	nlohmann::json result;
	result["tools"] = tools;

	nlohmann::json response;
	response["jsonrpc"] = "2.0";
	response["id"] = request.value("id", nlohmann::json());
	response["result"] = result;
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

	// voxedit_cmd_* tools
	if (toolName.rfind("voxedit_cmd_", 0) == 0) {
		const std::string &cmdName = toolName.substr(12);
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
	if (toolName.rfind("voxedit_script_", 0) == 0) {
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

	if (toolName == "voxedit_create_generator") {
		const std::string &name = args.value("name", "");
		const std::string &code = args.value("code", "");
		bool run = args.value("run", true);
		const std::string &scriptArgs = args.value("args", "");

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

	if (toolName == "voxedit_lua_api") {
		io::BufferedReadWriteStream stream;
		if (_luaApi.apiJsonToStream(stream)) {
			core::String json((const char *)stream.getBuffer(), (size_t)stream.size());
			sendToolResult(id, json);
		} else {
			sendToolResult(id, "Failed to generate Lua API documentation", true);
		}
		return;
	}

	if (toolName == "voxedit_get_scene_state") {
		if (!_sceneStateReceived) {
			sendToolResult(id, "Timeout waiting for scene state", true);
			return;
		}
		// Generate JSON from the cached scene graph
		io::BufferedReadWriteStream stream;
		scenegraph::sceneGraphJson(_sceneGraph, false, stream);
		core::String json((const char *)stream.getBuffer(), (size_t)stream.size());
		sendToolResult(id, json);
		return;
	}

	if (toolName == "voxedit_place_voxels") {
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

	if (toolName == "voxedit_get_palette") {
		if (!args.contains("nodeUUID") || !args["nodeUUID"].is_string()) {
			sendToolResult(id, "Missing nodeUUID argument", true);
			return;
		}

		const core::UUID nodeUUID(args["nodeUUID"].get<std::string>().c_str());
		if (!nodeUUID.isValid()) {
			sendToolResult(id, "Invalid node UUID", true);
			return;
		}

		scenegraph::SceneGraphNode *node = _sceneGraph.findNodeByUUID(nodeUUID);
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
			paletteJson["colors"].push_back(colorJson);
		}
		sendToolResult(id, paletteJson.dump().c_str());
		return;
	}

	if (toolName == "voxedit_find_color") {
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

		scenegraph::SceneGraphNode *node = _sceneGraph.findNodeByUUID(nodeUUID);
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
	response["result"] = result;
	sendResponse(response);
}

void McpServer::sendResponse(const nlohmann::json &response) {
	const core::String &out = response.dump().c_str();
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
