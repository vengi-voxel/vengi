/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "network/MessageStream.h"
#include "network/SocketId.h"

namespace voxedit {

struct RemoteClient {
	explicit RemoteClient(network::SocketId _socket) : socket(_socket) {
	}
	RemoteClient(RemoteClient &&other) noexcept;
	RemoteClient &operator=(RemoteClient &&other) noexcept;

	RemoteClient(const RemoteClient &) = delete;
	RemoteClient &operator=(const RemoteClient &) = delete;

	network::SocketId socket;
	uint64_t bytesIn = 0u;
	uint64_t bytesOut = 0u;
	double lastPingTime = 0.0;
	double lastActivity = 0.0;
	network::MessageStream in;
	network::MessageStream out;
	core::String name;
};
using RemoteClients = core::DynamicArray<RemoteClient>;

} // namespace voxedit
