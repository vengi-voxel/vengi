#pragma once

#include "Tool.h"
#include "command/Command.h"

namespace voxedit {

/**
 * @brief Run local commands to modify the scene graph - any change is sent to the server via network
 */
class CommandTool : public Tool {
private:
	core::String _cmdName;
	core::DynamicArray<command::CommandArg> _args;

	static core::String toolName(const command::Command &cmd);

public:
	CommandTool(const command::Command &cmd);
	~CommandTool() override = default;

	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
