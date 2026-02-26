/**
 * @file
 */

#include "ScriptTool.h"
#include "core/StringUtil.h"

namespace voxedit {

core::String ScriptTool::toolName(const voxedit::LuaScriptInfo &info) {
	const core::String &name = core::string::extractFilename(info.filename);
	return core::String::format("voxedit_script_%s", name.c_str());
}

ScriptTool::ScriptTool(const voxedit::LuaScriptInfo &info) : Tool(toolName(info)), _info(info) {
	_scriptName = info.filename;
	if (info.description.empty()) {
		_tool["description"] = info.filename.c_str();
	} else {
		_tool["description"] = info.description.c_str();
	}

	nlohmann::json inputSchema;
	inputSchema["type"] = "object";
	nlohmann::json properties = nlohmann::json::object();
	nlohmann::json required = nlohmann::json::array();

	for (const voxedit::LuaParameterInfo &param : info.parameters) {
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
		case voxedit::LuaParameterType::HexColor:
			propSchema["type"] = "string";
			propSchema["pattern"] = "^#?[0-9a-fA-F]{6,8}$";
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
				propSchema["enum"] = core::move(enumArray);
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
		} else {
			propSchema["description"] = param.name.c_str();
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
		properties[param.name.c_str()] = core::move(propSchema);
		// All script parameters are required unless they have a default value
		if (param.defaultValue.empty()) {
			required.push_back(param.name.c_str());
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

bool ScriptTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	std::string builtArgs;
	if (args.is_object() && args.contains("args") && args["args"].is_string()) {
		builtArgs = args["args"].get<std::string>();
	} else {
		for (const voxedit::LuaParameterInfo &p : _info.parameters) {
			if (args.contains(p.name.c_str())) {
				if (args[p.name.c_str()].is_string()) {
					if (!builtArgs.empty()) {
						builtArgs += " ";
					}
					builtArgs += args[p.name.c_str()].get<std::string>();
				} else if (args[p.name.c_str()].is_number()) {
					if (!builtArgs.empty()) {
						builtArgs += " ";
					}
					builtArgs += std::to_string(args[p.name.c_str()].get<double>());
				} else if (args[p.name.c_str()].is_boolean()) {
					if (!builtArgs.empty()) {
						builtArgs += " ";
					}
					builtArgs += args[p.name.c_str()].get<bool>() ? "1" : "0";
				}
			} else if (!p.defaultValue.empty()) {
				if (!builtArgs.empty()) {
					builtArgs += " ";
				}
				builtArgs += p.defaultValue.c_str();
			} else {
				return ctx.result(id, core::String::format("Missing required parameter '%s'", p.name.c_str()), true);
			}
		}
	}

	const core::String &cmd = core::String::format("xs %s %s", _scriptName.c_str(), builtArgs.c_str());
	return sendCommand(ctx, cmd, id);
}

} // namespace voxedit
