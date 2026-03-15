/**
 * @file
 */

#include "ScriptTool.h"
#include "core/StringUtil.h"
#include "voxedit-util/network/protocol/LuaScriptExecMessage.h"

namespace voxedit {

core::String ScriptTool::toolName(const voxedit::LuaScriptInfo &info) {
	const core::String &name = core::string::extractFilename(info.filename);
	return core::String::format("voxedit_script_%s", name.c_str());
}

ScriptTool::ScriptTool(const voxedit::LuaScriptInfo &info) : Tool(toolName(info)), _info(info) {
	_scriptName = info.filename;
	if (info.description.empty()) {
		_tool.set("description", info.filename.c_str());
	} else {
		_tool.set("description", info.description.c_str());
	}

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json properties = json::Json::object();
	json::Json required = json::Json::array();

	for (const voxedit::LuaParameterInfo &param : info.parameters) {
		json::Json propSchema = json::Json::object();
		switch (param.type) {
		case voxedit::LuaParameterType::Integer:
		case voxedit::LuaParameterType::ColorIndex:
			propSchema.set("type", "integer");
			if (param.minValue <= param.maxValue) {
				propSchema.set("minimum", (int)param.minValue);
				propSchema.set("maximum", (int)param.maxValue);
			}
			break;
		case voxedit::LuaParameterType::HexColor:
			propSchema.set("type", "string");
			propSchema.set("pattern", "^#?[0-9a-fA-F]{6,8}$");
			break;
		case voxedit::LuaParameterType::Float:
			propSchema.set("type", "number");
			if (param.minValue <= param.maxValue) {
				propSchema.set("minimum", param.minValue);
				propSchema.set("maximum", param.maxValue);
			}
			break;
		case voxedit::LuaParameterType::Boolean:
			propSchema.set("type", "boolean");
			break;
		case voxedit::LuaParameterType::Enum:
			propSchema.set("type", "string");
			if (!param.enumValues.empty()) {
				json::Json enumArray = json::Json::array();
				core::DynamicArray<core::String> values;
				core::string::splitString(param.enumValues, values, ";");
				for (const core::String &v : values) {
					enumArray.push(v.c_str());
				}
				propSchema.set("enum", core::move(enumArray));
			}
			break;
		case voxedit::LuaParameterType::String:
		case voxedit::LuaParameterType::File:
		default:
			propSchema.set("type", "string");
			break;
		}
		if (!param.description.empty()) {
			propSchema.set("description", param.description.c_str());
		} else {
			propSchema.set("description", param.name.c_str());
		}
		if (!param.defaultValue.empty()) {
			if (param.type == voxedit::LuaParameterType::Integer ||
				param.type == voxedit::LuaParameterType::ColorIndex) {
				propSchema.set("default", core::string::toInt(param.defaultValue));
			} else if (param.type == voxedit::LuaParameterType::Float) {
				propSchema.set("default", core::string::toFloat(param.defaultValue));
			} else if (param.type == voxedit::LuaParameterType::Boolean) {
				propSchema.set("default", core::string::toBool(param.defaultValue));
			} else {
				propSchema.set("default", param.defaultValue.c_str());
			}
		}
		properties.set(param.name.c_str(), core::move(propSchema));
		// All script parameters are required unless they have a default value
		if (param.defaultValue.empty()) {
			required.push(param.name.c_str());
		}
	}

	if (!properties.empty()) {
		inputSchema.set("properties", core::move(properties));
	}
	if (!required.empty()) {
		inputSchema.set("required", core::move(required));
	}
	_tool.set("inputSchema", core::move(inputSchema));
}

bool ScriptTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	core::String builtArgs;
	if (args.isObject() && args.contains("args") && args.get("args").isString()) {
		builtArgs = args.get("args").str().c_str();
	} else {
		for (const voxedit::LuaParameterInfo &p : _info.parameters) {
			if (args.contains(p.name.c_str())) {
				const json::Json val = args.get(p.name.c_str());
				if (val.isString()) {
					if (!builtArgs.empty()) {
						builtArgs += " ";
					}
					builtArgs += val.str().c_str();
				} else if (val.isNumber()) {
					if (!builtArgs.empty()) {
						builtArgs += " ";
					}
					builtArgs += core::String::format("%g", val.doubleVal());
				} else if (val.isBool()) {
					if (!builtArgs.empty()) {
						builtArgs += " ";
					}
					builtArgs += val.boolVal() ? "1" : "0";
				}
			} else if (!p.defaultValue.empty()) {
				if (!builtArgs.empty()) {
					builtArgs += " ";
				}
				builtArgs += p.defaultValue;
			} else {
				return ctx.result(id, core::String::format("Missing required parameter '%s'", p.name.c_str()), true);
			}
		}
	}

	const core::String scriptName = _scriptName;
	if (!sendMessage(ctx, voxedit::LuaScriptExecMessage(scriptName, builtArgs, rconPassword()))) {
		return ctx.result(id, core::String::format("Failed to send script exec for %s", scriptName.c_str()), true);
	}

	core::DynamicArray<core::String> logs;
	bool hadErrors = false;
	const bool ackReceived = waitForServerResponse(ctx, logs, hadErrors);

	core::String result;
	if (ackReceived) {
		result = core::String::format("Executed script: %s", scriptName.c_str());
	} else {
		result = core::String::format("Executed script: %s (timeout waiting for server response)", scriptName.c_str());
	}

	for (const core::String &log : logs) {
		result.append("\n");
		result.append(log);
	}

	return ctx.result(id, result, hadErrors);
}

} // namespace voxedit
