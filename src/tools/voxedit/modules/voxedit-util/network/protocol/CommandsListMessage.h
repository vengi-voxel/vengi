/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Info about a single command
 */
struct CommandInfo {
	core::String name;
	core::String description;
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
		}
		writeSize();
	}

	const core::DynamicArray<CommandInfo> &commands() const {
		return _commands;
	}
};

} // namespace voxedit
