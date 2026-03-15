/**
 * @file
 */

#include "CommandTool.h"
#include "command/Command.h"
#include "core/StringUtil.h"

namespace voxedit {

CommandTool::CommandTool() : Tool("voxedit_cmd") {
	_tool.set("description", "Execute a command on the voxedit server by name with optional arguments. Use voxedit_cmd_list to discover commands.");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");

	json::Json properties = json::Json::object();
	properties.set("command", propTypeDescription("string", "Command name to execute"));

	json::Json argsProp = json::Json::object();
	argsProp.set("type", "array");
	argsProp.set("description", "Command arguments as strings in order");
	json::Json itemSchema = json::Json::object();
	itemSchema.set("type", "string");
	argsProp.set("items", core::move(itemSchema));
	properties.set("args", core::move(argsProp));

	inputSchema.set("properties", core::move(properties));
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("command");
	inputSchema.set("required", _requiredArr);
	_tool.set("inputSchema", core::move(inputSchema));
}

bool CommandTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	if (!args.contains("command") || !args.get("command").isString()) {
		return ctx.result(id, "Missing required parameter 'command'", true);
	}

	core::String cmd = args.get("command").str().c_str();
	core::DynamicArray<core::String> rawArgs;
	if (args.contains("args") && args.get("args").isArray()) {
		rawArgs.reserve(args.get("args").size());
		for (const auto &arg : args.get("args")) {
			if (arg.isString()) {
				rawArgs.push_back(arg.str().c_str());
			} else if (arg.isNumberInteger()) {
				rawArgs.push_back(core::String::format("%d", arg.intVal()));
			} else if (arg.isNumberFloat()) {
				rawArgs.push_back(core::String::format("%f", arg.doubleVal()));
			} else if (arg.isBool()) {
				rawArgs.push_back(arg.boolVal() ? "true" : "false");
			}
		}
	}

	core::String fullCmd = cmd;
	for (const core::String &arg : rawArgs) {
		fullCmd.append(" ");
		fullCmd.append(arg);
	}
	return sendCommand(ctx, fullCmd, id);
}

CommandListTool::CommandListTool() : Tool("voxedit_cmd_list") {
	_tool.set("description", "List available commands with help text and arguments. Optionally filter by name prefix.");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");

	json::Json properties = json::Json::object();
	properties.set("filter", propTypeDescription("string", "Optional prefix to filter commands"));

	inputSchema.set("properties", core::move(properties));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool CommandListTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	core::String filter;
	if (args.contains("filter") && args.get("filter").isString()) {
		filter = args.get("filter").str().c_str();
	}

	json::Json commands = json::Json::array();
	command::Command::visitSorted([&](const command::Command &c) {
		if (c.isInput()) {
			return;
		}
		if (!filter.empty() && !core::string::startsWith(c.name(), filter)) {
			return;
		}
		json::Json entry = json::Json::object();
		entry.set("name", c.name().c_str());
		if (!c.help().empty()) {
			entry.set("help", c.help().c_str());
		}
		if (!c.args().empty()) {
			json::Json cmdArgs = json::Json::array();
			for (const command::CommandArg &arg : c.args()) {
				json::Json a = json::Json::object();
				a.set("name", arg.name.c_str());
				if (!arg.description.empty()) {
					a.set("desc", arg.description.c_str());
				}
				switch (arg.type) {
				case command::ArgType::String:
					a.set("type", "string");
					break;
				case command::ArgType::Int:
					a.set("type", "int");
					break;
				case command::ArgType::Float:
					a.set("type", "float");
					break;
				case command::ArgType::Bool:
					a.set("type", "bool");
					break;
				}
				a.set("optional", arg.optional);
				if (!arg.defaultVal.empty()) {
					a.set("default", arg.defaultVal.c_str());
				}
				cmdArgs.push(a);
			}
			entry.set("args", core::move(cmdArgs));
		}
		commands.push(entry);
	});

	return ctx.result(id, commands.dump().c_str(), false);
}

} // namespace voxedit
