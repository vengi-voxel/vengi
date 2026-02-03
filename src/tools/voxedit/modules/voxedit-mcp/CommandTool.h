#pragma once

#include "Tool.h"
#include "voxedit-util/network/protocol/CommandsListMessage.h"

namespace voxedit {

class CommandTool : public Tool {
private:
	core::String _cmdName;
	voxedit::CommandInfo _info;

public:
	static core::String toolName(const voxedit::CommandInfo &info);

	CommandTool(const voxedit::CommandInfo &info);
	~CommandTool() override = default;

	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
