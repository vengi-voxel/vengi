/**
 * @file
 */

#include "CommandTool.h"
#include "command/Command.h"
#include "core/StringUtil.h"

namespace voxedit {

CommandTool::CommandTool(const command::Command &info) : Tool("voxedit_cmd_" + info.name()), _cmdName(info.name()), _args(info.args()) {
	_tool["description"] = info.help().c_str();

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	nlohmann::json properties = nlohmann::json::object();
	nlohmann::json required = nlohmann::json::array();

	for (const command::CommandArg &arg : info.args()) {
		nlohmann::json propSchema;
		switch (arg.type) {
		case command::ArgType::Int:
			propSchema["type"] = "integer";
			break;
		case command::ArgType::Float:
			propSchema["type"] = "number";
			break;
		case command::ArgType::Bool:
			propSchema["type"] = "boolean";
			break;
		case command::ArgType::String:
		default:
			propSchema["type"] = "string";
			break;
		}
		if (!arg.description.empty()) {
			propSchema["description"] = arg.description.c_str();
		} else {
			propSchema["description"] = arg.name.c_str();
		}
		if (!arg.defaultVal.empty()) {
			if (arg.type == command::ArgType::Int) {
				propSchema["default"] = core::string::toInt(arg.defaultVal);
			} else if (arg.type == command::ArgType::Float) {
				propSchema["default"] = core::string::toFloat(arg.defaultVal);
			} else if (arg.type == command::ArgType::Bool) {
				propSchema["default"] = core::string::toBool(arg.defaultVal);
			} else {
				propSchema["default"] = arg.defaultVal.c_str();
			}
		}
		properties[arg.name.c_str()] = core::move(propSchema);
		// Required if not optional and no default value
		if (!arg.optional && arg.defaultVal.empty()) {
			required.push_back(arg.name.c_str());
		}
	}

	if (!properties.empty()) {
		inputSchema["properties"] = core::move(properties);
	}
	if (!required.empty()) {
		inputSchema["required"] = core::move(required);
	}
	_tool["inputSchema"] = core::move(inputSchema);
}

bool CommandTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	core::String cmd = _cmdName;

	for (const command::CommandArg &p : _args) {
		if (args.contains(p.name.c_str())) {
			const auto &argVal = args[p.name.c_str()];
			if (argVal.is_string()) {
				cmd.append(" ");
				cmd.append(argVal.get<std::string>().c_str());
			} else if (argVal.is_number_integer()) {
				cmd.append(" ");
				cmd.append(core::String::format("%d", argVal.get<int>()));
			} else if (argVal.is_number_float()) {
				cmd.append(" ");
				cmd.append(core::String::format("%f", argVal.get<double>()));
			} else if (argVal.is_boolean()) {
				cmd.append(" ");
				cmd.append(argVal.get<bool>() ? "true" : "false");
			}
		} else if (!p.defaultVal.empty()) {
			cmd.append(" ");
			cmd.append(p.defaultVal);
		} else if (!p.optional) {
			return ctx.result(id, core::String::format("Missing required parameter '%s'", p.name.c_str()), true);
		}
	}
	if (command::Command::execute(cmd) > 1) {
		return ctx.result(id, core::String::format("Executed command '%s'", cmd.c_str()), false);
	}
	return ctx.result(id, core::String::format("Failed to execute command '%s'", cmd.c_str()), true);
}

} // namespace voxedit
