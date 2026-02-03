/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Argument types for command parameters (mirrored from command::ArgType)
 */
enum class CommandArgType : uint8_t {
	String,
	Int,
	Float,
	Bool
};

/**
 * @brief Info about a single command argument (network serializable subset of command::CommandArg)
 */
struct CommandArgInfo {
	core::String name;
	core::String description;
	core::String defaultVal;
	CommandArgType type = CommandArgType::String;
	bool optional = false;
};

/**
 * @brief Info about a single command
 */
struct CommandInfo {
	core::String name;
	core::String description;
	core::DynamicArray<CommandArgInfo> args;
};

/**
 * @brief Response containing the list of available commands
 */
class CommandsListMessage : public network::ProtocolMessage {
private:
	core::DynamicArray<CommandInfo> _commands;

public:
	CommandsListMessage(const core::DynamicArray<CommandInfo> &commands) : ProtocolMessage(PROTO_COMMANDS_LIST) {
		if (!writeUInt32((uint32_t)commands.size())) {
			Log::error("Failed to write command count in CommandsListMessage ctor");
			return;
		}
		for (const auto &cmd : commands) {
			if (!writePascalStringUInt16LE(cmd.name)) {
				Log::error("Failed to write command name in CommandsListMessage ctor");
				return;
			}
			if (!writePascalStringUInt16LE(cmd.description)) {
				Log::error("Failed to write command description in CommandsListMessage ctor");
				return;
			}
			if (!writeArgs(cmd.args)) {
				return;
			}
		}
		writeSize();
	}

	CommandsListMessage(network::MessageStream &in) {
		_id = PROTO_COMMANDS_LIST;
		uint32_t count = 0;
		if (in.readUInt32(count) == -1) {
			Log::error("Failed to read command count");
			return;
		}
		_commands.reserve(count);
		for (uint32_t i = 0; i < count; ++i) {
			CommandInfo info;
			if (!in.readPascalStringUInt16LE(info.name)) {
				Log::error("Failed to read command name");
				return;
			}
			if (!in.readPascalStringUInt16LE(info.description)) {
				Log::error("Failed to read command description");
				return;
			}
			if (!readArgs(in, info.args)) {
				return;
			}
			_commands.push_back(info);
		}
	}

	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in CommandsListMessage::writeBack");
			return;
		}
		if (!writeUInt32((uint32_t)_commands.size())) {
			Log::error("Failed to write command count in CommandsListMessage::writeBack");
			return;
		}
		for (const auto &cmd : _commands) {
			if (!writePascalStringUInt16LE(cmd.name)) {
				Log::error("Failed to write command name in CommandsListMessage::writeBack");
				return;
			}
			if (!writePascalStringUInt16LE(cmd.description)) {
				Log::error("Failed to write command description in CommandsListMessage::writeBack");
				return;
			}
			if (!writeArgs(cmd.args)) {
				return;
			}
		}
		writeSize();
	}

	const core::DynamicArray<CommandInfo> &commands() const {
		return _commands;
	}

private:
	bool writeArgs(const core::DynamicArray<CommandArgInfo> &args) {
		if (!writeUInt16((uint16_t)args.size())) {
			Log::error("Failed to write args count");
			return false;
		}
		for (const auto &arg : args) {
			if (!writePascalStringUInt16LE(arg.name)) {
				Log::error("Failed to write arg name");
				return false;
			}
			if (!writePascalStringUInt16LE(arg.description)) {
				Log::error("Failed to write arg description");
				return false;
			}
			if (!writePascalStringUInt16LE(arg.defaultVal)) {
				Log::error("Failed to write arg default value");
				return false;
			}
			if (!writeUInt8((uint8_t)arg.type)) {
				Log::error("Failed to write arg type");
				return false;
			}
			if (!writeBool(arg.optional)) {
				Log::error("Failed to write arg optional flag");
				return false;
			}
		}
		return true;
	}

	static bool readArgs(network::MessageStream &in, core::DynamicArray<CommandArgInfo> &args) {
		uint16_t argCount = 0;
		if (in.readUInt16(argCount) == -1) {
			Log::error("Failed to read args count");
			return false;
		}
		args.reserve(argCount);
		for (uint16_t j = 0; j < argCount; ++j) {
			CommandArgInfo arg;
			if (!in.readPascalStringUInt16LE(arg.name)) {
				Log::error("Failed to read arg name");
				return false;
			}
			if (!in.readPascalStringUInt16LE(arg.description)) {
				Log::error("Failed to read arg description");
				return false;
			}
			if (!in.readPascalStringUInt16LE(arg.defaultVal)) {
				Log::error("Failed to read arg default value");
				return false;
			}
			uint8_t typeVal = 0;
			if (in.readUInt8(typeVal) == -1) {
				Log::error("Failed to read arg type");
				return false;
			}
			arg.type = (CommandArgType)typeVal;
			arg.optional = in.readBool();
			args.push_back(arg);
		}
		return true;
	}
};

} // namespace voxedit
