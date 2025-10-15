/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "voxedit-util/network/ProtocolMessage.h"

namespace voxedit {
namespace network {

/**
 * @brief Allows remote command execution on the server side
 */
class CommandMessage : public ProtocolMessage {
private:
	core::String _rconPassword;
	core::String _command;

public:
	CommandMessage(const core::String &command, const core::String &rconPassword) : ProtocolMessage(PROTO_COMMAND) {
		_rconPassword = rconPassword;
		_command = command;

		if (!writePascalStringUInt16LE(_rconPassword)) {
			Log::error("Failed to write rcon password in CommandMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(_command)) {
			Log::error("Failed to write command in CommandMessage ctor");
			return;
		}
		writeSize();
	}

	CommandMessage(MessageStream &in) {
		_id = PROTO_COMMAND;
		if (!in.readPascalStringUInt16LE(_rconPassword)) {
			Log::error("Failed to read rcon password");
			return;
		}
		if (!in.readPascalStringUInt16LE(_command)) {
			Log::error("Failed to read command");
			return;
		}
	}
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in CommandMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_rconPassword)) {
			Log::error("Failed to write rcon password CommandMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_command)) {
			Log::error("Failed to write command CommandMessage::writeBack");
			return;
		}
		writeSize();
	}

	const core::String &command() const {
		return _command;
	}

	const core::String &rconPassword() const {
		return _rconPassword;
	}
};

} // namespace network
} // namespace voxedit
