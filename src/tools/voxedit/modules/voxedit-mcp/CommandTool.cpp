/**
 * @file
 */

#include "CommandTool.h"
#include "command/Command.h"
#include "core/StringUtil.h"

namespace voxedit {

CommandTool::CommandTool() : Tool("voxedit_cmd") {
	_tool["description"] = "Execute a command by name with optional arguments. Use voxedit_cmd_list to discover commands.";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";

	nlohmann::json properties = nlohmann::json::object();
	properties["command"] = propTypeDescription("string", "Command name to execute");

	nlohmann::json argsProp;
	argsProp["type"] = "array";
	argsProp["description"] = "Command arguments as strings in order";
	nlohmann::json itemSchema;
	itemSchema["type"] = "string";
	argsProp["items"] = core::move(itemSchema);
	properties["args"] = core::move(argsProp);

	inputSchema["properties"] = core::move(properties);
	inputSchema["required"] = nlohmann::json::array({"command"});
	_tool["inputSchema"] = core::move(inputSchema);
}

bool CommandTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	if (!args.contains("command") || !args["command"].is_string()) {
		return ctx.result(id, "Missing required parameter 'command'", true);
	}

	core::String cmd = args["command"].get<std::string>().c_str();
	core::DynamicArray<core::String> rawArgs;
	if (args.contains("args") && args["args"].is_array()) {
		rawArgs.reserve(args["args"].size());
		for (const auto &arg : args["args"]) {
			if (arg.is_string()) {
				rawArgs.push_back(arg.get<std::string>().c_str());
			} else if (arg.is_number_integer()) {
				rawArgs.push_back(core::String::format("%d", arg.get<int>()));
			} else if (arg.is_number_float()) {
				rawArgs.push_back(core::String::format("%f", arg.get<double>()));
			} else if (arg.is_boolean()) {
				rawArgs.push_back(arg.get<bool>() ? "true" : "false");
			}
		}
	}

	if (command::Command::execute(cmd, rawArgs)) {
		return ctx.result(id, core::String::format("Executed command '%s'", cmd.c_str()), false);
	}
	return ctx.result(id, core::String::format("Failed to execute command '%s'", cmd.c_str()), true);
}

CommandListTool::CommandListTool() : Tool("voxedit_cmd_list") {
	_tool["description"] = "List available commands with help text and arguments. Optionally filter by name prefix.";

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";

	nlohmann::json properties = nlohmann::json::object();
	properties["filter"] = propTypeDescription("string", "Optional prefix to filter commands");

	inputSchema["properties"] = core::move(properties);
	_tool["inputSchema"] = core::move(inputSchema);
}

bool CommandListTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	core::String filter;
	if (args.contains("filter") && args["filter"].is_string()) {
		filter = args["filter"].get<std::string>().c_str();
	}

	nlohmann::json commands = nlohmann::json::array();
	command::Command::visitSorted([&](const command::Command &c) {
		if (c.isInput()) {
			return;
		}
		if (!filter.empty() && !core::string::startsWith(c.name(), filter)) {
			return;
		}
		nlohmann::json entry;
		entry["name"] = c.name().c_str();
		if (!c.help().empty()) {
			entry["help"] = c.help().c_str();
		}
		if (!c.args().empty()) {
			nlohmann::json cmdArgs = nlohmann::json::array();
			for (const command::CommandArg &arg : c.args()) {
				nlohmann::json a;
				a["name"] = arg.name.c_str();
				if (!arg.description.empty()) {
					a["desc"] = arg.description.c_str();
				}
				switch (arg.type) {
				case command::ArgType::String:
					a["type"] = "string";
					break;
				case command::ArgType::Int:
					a["type"] = "int";
					break;
				case command::ArgType::Float:
					a["type"] = "float";
					break;
				case command::ArgType::Bool:
					a["type"] = "bool";
					break;
				}
				a["optional"] = arg.optional;
				if (!arg.defaultVal.empty()) {
					a["default"] = arg.defaultVal.c_str();
				}
				cmdArgs.emplace_back(core::move(a));
			}
			entry["args"] = core::move(cmdArgs);
		}
		commands.emplace_back(core::move(entry));
	});

	return ctx.result(id, commands.dump().c_str(), false);
}

} // namespace voxedit
