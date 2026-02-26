/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

enum class LuaParameterType : uint8_t {
	String,
	Integer,
	Float,
	Boolean,
	ColorIndex,
	Enum,
	File,
	HexColor,
	Max
};

/**
 * @brief Info about a script parameter
 */
struct LuaParameterInfo {
	core::String name;
	core::String description;
	core::String defaultValue;
	core::String enumValues; /**< separated by ; */
	double minValue = 1.0;
	double maxValue = 0.0;
	LuaParameterType type = LuaParameterType::Max;
};

/**
 * @brief Info about a single lua script
 */
struct LuaScriptInfo {
	core::String filename;
	core::String description;
	bool valid = false;
	core::DynamicArray<LuaParameterInfo> parameters;
};

/**
 * @brief Response containing the list of available lua scripts
 */
class LuaScriptsListMessage : public network::ProtocolMessage {
private:
	core::DynamicArray<LuaScriptInfo> _scripts;

public:
	LuaScriptsListMessage(const core::DynamicArray<LuaScriptInfo> &scripts) : ProtocolMessage(PROTO_LUA_SCRIPTS_LIST) {
		if (!writeUInt32((uint32_t)scripts.size())) {
			Log::error("Failed to write script count in LuaScriptsListMessage ctor");
			return;
		}
		for (const auto &script : scripts) {
			if (!writePascalStringUInt16LE(script.filename)) {
				Log::error("Failed to write script filename in LuaScriptsListMessage ctor");
				return;
			}
			if (!writePascalStringUInt16LE(script.description)) {
				Log::error("Failed to write script description in LuaScriptsListMessage ctor");
				return;
			}
			if (!writeBool(script.valid)) {
				Log::error("Failed to write script valid flag in LuaScriptsListMessage ctor");
				return;
			}
			if (!writeParameters(script.parameters)) {
				return;
			}
		}
		writeSize();
	}

	LuaScriptsListMessage(network::MessageStream &in) {
		_id = PROTO_LUA_SCRIPTS_LIST;
		uint32_t count = 0;
		if (in.readUInt32(count) == -1) {
			Log::error("Failed to read script count");
			return;
		}
		_scripts.reserve(count);
		for (uint32_t i = 0; i < count; ++i) {
			LuaScriptInfo info;
			if (!in.readPascalStringUInt16LE(info.filename)) {
				Log::error("Failed to read script filename");
				return;
			}
			if (!in.readPascalStringUInt16LE(info.description)) {
				Log::error("Failed to read script description");
				return;
			}
			info.valid = in.readBool();
			if (!readParameters(in, info.parameters)) {
				return;
			}
			_scripts.push_back(info);
		}
	}

	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in LuaScriptsListMessage::writeBack");
			return;
		}
		if (!writeUInt32((uint32_t)_scripts.size())) {
			Log::error("Failed to write script count in LuaScriptsListMessage::writeBack");
			return;
		}
		for (const auto &script : _scripts) {
			if (!writePascalStringUInt16LE(script.filename)) {
				Log::error("Failed to write script filename in LuaScriptsListMessage::writeBack");
				return;
			}
			if (!writePascalStringUInt16LE(script.description)) {
				Log::error("Failed to write script description in LuaScriptsListMessage::writeBack");
				return;
			}
			if (!writeBool(script.valid)) {
				Log::error("Failed to write script valid flag in LuaScriptsListMessage::writeBack");
				return;
			}
			if (!writeParameters(script.parameters)) {
				return;
			}
		}
		writeSize();
	}

	const core::DynamicArray<LuaScriptInfo> &scripts() const {
		return _scripts;
	}

private:
	bool writeParameters(const core::DynamicArray<LuaParameterInfo> &params) {
		if (!writeUInt16((uint16_t)params.size())) {
			Log::error("Failed to write parameter count");
			return false;
		}
		for (const auto &param : params) {
			if (!writePascalStringUInt16LE(param.name)) {
				Log::error("Failed to write parameter name");
				return false;
			}
			if (!writePascalStringUInt16LE(param.description)) {
				Log::error("Failed to write parameter description");
				return false;
			}
			if (!writePascalStringUInt16LE(param.defaultValue)) {
				Log::error("Failed to write parameter default value");
				return false;
			}
			if (!writePascalStringUInt16LE(param.enumValues)) {
				Log::error("Failed to write parameter enum values");
				return false;
			}
			if (!writeDouble(param.minValue)) {
				Log::error("Failed to write parameter min value");
				return false;
			}
			if (!writeDouble(param.maxValue)) {
				Log::error("Failed to write parameter max value");
				return false;
			}
			if (!writeUInt8((uint8_t)param.type)) {
				Log::error("Failed to write parameter type");
				return false;
			}
		}
		return true;
	}

	static bool readParameters(network::MessageStream &in, core::DynamicArray<LuaParameterInfo> &params) {
		uint16_t paramCount = 0;
		if (in.readUInt16(paramCount) == -1) {
			Log::error("Failed to read parameter count");
			return false;
		}
		params.reserve(paramCount);
		for (uint16_t j = 0; j < paramCount; ++j) {
			LuaParameterInfo param;
			if (!in.readPascalStringUInt16LE(param.name)) {
				Log::error("Failed to read parameter name");
				return false;
			}
			if (!in.readPascalStringUInt16LE(param.description)) {
				Log::error("Failed to read parameter description");
				return false;
			}
			if (!in.readPascalStringUInt16LE(param.defaultValue)) {
				Log::error("Failed to read parameter default value");
				return false;
			}
			if (!in.readPascalStringUInt16LE(param.enumValues)) {
				Log::error("Failed to read parameter enum values");
				return false;
			}
			if (in.readDouble(param.minValue) == -1) {
				Log::error("Failed to read parameter min value");
				return false;
			}
			if (in.readDouble(param.maxValue) == -1) {
				Log::error("Failed to read parameter max value");
				return false;
			}
			uint8_t typeVal = 0;
			if (in.readUInt8(typeVal) == -1) {
				Log::error("Failed to read parameter type");
				return false;
			}
			param.type = (LuaParameterType)typeVal;
			params.push_back(param);
		}
		return true;
	}
};

} // namespace voxedit
