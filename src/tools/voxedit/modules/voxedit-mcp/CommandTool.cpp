/**
 * @file
 */

#include "CommandTool.h"
#include "command/Command.h"

namespace voxedit {

core::String CommandTool::toolName(const voxedit::CommandInfo &info) {
	const core::String &cmd = info.name;
	if (cmd[0] == COMMAND_PRESSED[0]) {
		return core::String::format("voxedit_cmd_pressed_%s", cmd.c_str() + 1);
	} else if (cmd[0] == COMMAND_RELEASED[0]) {
		return core::String::format("voxedit_cmd_released_%s", cmd.c_str() + 1);
	}
	return core::String("voxedit_cmd_" + cmd);
}

CommandTool::CommandTool(const voxedit::CommandInfo &info) : Tool(toolName(info)) {
	_cmdName = info.name;
	_tool["description"] = info.description.c_str();

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	inputSchema["properties"]["args"] = propTypeDescription("string", "Command arguments");
	_tool["inputSchema"] = core::move(inputSchema);
}

bool CommandTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const std::string &cmdArgs = args.value("args", "");
	const core::String &cmd = core::String::format("%s %s", _cmdName.c_str(), cmdArgs.c_str());
	return sendCommand(ctx, cmd, id);
}

} // namespace voxedit
