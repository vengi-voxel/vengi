/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

class ChatMessage : public network::ProtocolMessage {
private:
	core::String _sender;
	core::String _message;
	bool _system = false;

public:
	/**
	 * @brief Construct a chat message from a client (sender will be filled in by the server)
	 */
	ChatMessage(const core::String &message) : ProtocolMessage(PROTO_CHAT) {
		_message = message;
		if (!writePascalStringUInt16LE(_sender)) {
			Log::error("Failed to write sender in ChatMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(_message)) {
			Log::error("Failed to write message in ChatMessage ctor");
			return;
		}
		if (!writeBool(_system)) {
			Log::error("Failed to write system flag in ChatMessage ctor");
			return;
		}
		writeSize();
	}

	/**
	 * @brief Construct a chat message with sender info (used by server to broadcast)
	 */
	ChatMessage(const core::String &sender, const core::String &message, bool system)
		: ProtocolMessage(PROTO_CHAT), _sender(sender), _message(message), _system(system) {
		if (!writePascalStringUInt16LE(_sender)) {
			Log::error("Failed to write sender in ChatMessage ctor");
			return;
		}
		if (!writePascalStringUInt16LE(_message)) {
			Log::error("Failed to write message in ChatMessage ctor");
			return;
		}
		if (!writeBool(_system)) {
			Log::error("Failed to write system flag in ChatMessage ctor");
			return;
		}
		writeSize();
	}

	ChatMessage(network::MessageStream &in) {
		_id = PROTO_CHAT;
		if (!in.readPascalStringUInt16LE(_sender)) {
			Log::error("Failed to read sender");
			return;
		}
		if (!in.readPascalStringUInt16LE(_message)) {
			Log::error("Failed to read message");
			return;
		}
		_system = in.readBool();
	}

	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in ChatMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_sender)) {
			Log::error("Failed to write sender in ChatMessage::writeBack");
			return;
		}
		if (!writePascalStringUInt16LE(_message)) {
			Log::error("Failed to write message in ChatMessage::writeBack");
			return;
		}
		if (!writeBool(_system)) {
			Log::error("Failed to write system flag in ChatMessage::writeBack");
			return;
		}
		writeSize();
	}

	const core::String &sender() const {
		return _sender;
	}

	void setSender(const core::String &sender) {
		_sender = sender;
	}

	const core::String &message() const {
		return _message;
	}

	bool isSystem() const {
		return _system;
	}
};

} // namespace voxedit
