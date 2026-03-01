/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "core/String.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Server sends log messages to connected clients
 */
class LogMessage : public network::ProtocolMessage {
private:
	uint8_t _level;
	core::String _message;

public:
	LogMessage(Log::Level level, const core::String &message) : ProtocolMessage(PROTO_LOG) {
		_level = (uint8_t)level;
		_message = message;

		if (!writeUInt8(_level)) {
			Log::error("Failed to write log level in LogMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(_message)) {
			Log::error("Failed to write message in LogMessage ctor");
			return;
		}
		writeSize();
	}

	LogMessage(network::MessageStream &in) {
		_id = PROTO_LOG;
		if (in.readUInt8(_level) != 0) {
			Log::error("Failed to read log level");
			return;
		}
		if (!in.readPascalStringUInt16LE(_message)) {
			Log::error("Failed to read log message");
			return;
		}
	}

	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in LogMessage::writeBack");
			return;
		}
		if (!writeUInt8(_level)) {
			Log::error("Failed to write log level in LogMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_message)) {
			Log::error("Failed to write message in LogMessage::writeBack");
			return;
		}
		writeSize();
	}

	Log::Level level() const {
		return (Log::Level)_level;
	}

	const core::String &message() const {
		return _message;
	}
};

} // namespace voxedit
