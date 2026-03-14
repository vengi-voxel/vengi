#pragma once

#include "Tool.h"

namespace voxedit {

/**
 * @brief Single unified MCP tool that executes any registered command by name.
 * Replaces having one MCP tool per command to reduce MCP tool count.
 */
class CommandTool : public Tool {
public:
	CommandTool();
	~CommandTool() override = default;

	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

/**
 * @brief Lists available commands with their help text and arguments.
 */
class CommandListTool : public Tool {
public:
	CommandListTool();
	~CommandListTool() override = default;

	bool execute(const json::Json &id, const json::Json &args, ToolContext &ctx) override;
};

} // namespace voxedit
