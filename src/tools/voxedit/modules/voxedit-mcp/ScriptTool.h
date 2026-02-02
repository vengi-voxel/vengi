#pragma once

#include "Tool.h"
#include "voxedit-util/network/protocol/LuaScriptsListMessage.h"

namespace voxedit {

class ScriptTool : public Tool {
private:
	core::String _scriptName;
	voxedit::LuaScriptInfo _info;

public:
	ScriptTool(const voxedit::LuaScriptInfo &info);
	~ScriptTool() override = default;

    static core::String toolName(const voxedit::LuaScriptInfo &info);
	bool execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) override;
};

} // namespace voxedit
