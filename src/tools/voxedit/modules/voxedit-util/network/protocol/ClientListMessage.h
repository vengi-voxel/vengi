/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/network/ProtocolIds.h"
#include <stdint.h>

namespace voxedit {

/**
 * @brief A connected client entry with server-assigned id and display name.
 */
struct ClientInfo {
	uint8_t id;
	core::String name;
};

/**
 * @brief Message containing the list of connected clients (id + name).
 * Sent by server to all clients whenever the client list changes.
 */
class ClientListMessage : public network::ProtocolMessage {
private:
	core::DynamicArray<ClientInfo> _clients;

public:
	/**
	 * @brief Construct a client list message (server side)
	 */
	ClientListMessage(const core::DynamicArray<ClientInfo> &clients)
		: ProtocolMessage(PROTO_CLIENT_LIST), _clients(clients) {
		if (!writeUInt32((uint32_t)_clients.size())) {
			Log::error("Failed to write client count in ClientListMessage ctor");
			return;
		}
		for (size_t i = 0; i < _clients.size(); ++i) {
			if (!writeUInt8(_clients[i].id)) {
				Log::error("Failed to write client id %d in ClientListMessage ctor", (int)i);
				return;
			}
			if (!writePascalStringUInt16LE(_clients[i].name)) {
				Log::error("Failed to write client name %d in ClientListMessage ctor", (int)i);
				return;
			}
		}
		writeSize();
	}

	/**
	 * @brief Construct from incoming message stream (client side)
	 */
	ClientListMessage(network::MessageStream &in) {
		_id = PROTO_CLIENT_LIST;
		uint32_t count = 0;
		if (in.readUInt32(count) == -1) {
			Log::error("Failed to read client count");
			return;
		}
		_clients.reserve(count);
		for (uint32_t i = 0; i < count; ++i) {
			ClientInfo info;
			if (in.readUInt8(info.id) == -1) {
				Log::error("Failed to read client id %u", i);
				return;
			}
			if (!in.readPascalStringUInt16LE(info.name)) {
				Log::error("Failed to read client name %u", i);
				return;
			}
			_clients.push_back(info);
		}
	}

	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in ClientListMessage::writeBack");
			return;
		}
		if (!writeUInt32((uint32_t)_clients.size())) {
			Log::error("Failed to write client count in ClientListMessage::writeBack");
			return;
		}
		for (size_t i = 0; i < _clients.size(); ++i) {
			if (!writeUInt8(_clients[i].id)) {
				Log::error("Failed to write client id %d in ClientListMessage::writeBack", (int)i);
				return;
			}
			if (!writePascalStringUInt16LE(_clients[i].name)) {
				Log::error("Failed to write client name %d in ClientListMessage::writeBack", (int)i);
				return;
			}
		}
		writeSize();
	}

	const core::DynamicArray<ClientInfo> &clients() const {
		return _clients;
	}
};

} // namespace voxedit
