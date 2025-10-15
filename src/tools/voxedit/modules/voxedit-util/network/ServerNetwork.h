/**
 * @file
 */
#pragma once

#include "ProtocolHandler.h"
#include "SocketId.h"
#include "core/DeltaFrameSeconds.h"
#include "core/collection/DynamicArray.h"
#include "handler/server/InitSessionHandler.h"
#include "handler/server/SceneStateHandlerServer.h"
#include "voxedit-util/network/ProtocolHandlerRegistry.h"
#include "voxedit-util/network/handler/server/BroadcastHandler.h"
#include "voxedit-util/network/handler/server/CommandHandlerServer.h"

namespace voxedit {
namespace network {

class ProtocolMessage;
struct NetworkImpl;

struct RemoteClient {
	explicit RemoteClient(SocketId _socket) : socket(_socket) {
	}
	RemoteClient(RemoteClient &&other) noexcept;
	RemoteClient &operator=(RemoteClient &&other) noexcept;

	RemoteClient(const RemoteClient &) = delete;
	RemoteClient &operator=(const RemoteClient &) = delete;

	SocketId socket;
	uint64_t bytesIn = 0u;
	uint64_t bytesOut = 0u;
	double lastPingTime = 0.0;
	double lastActivity = 0.0;
	MessageStream in;
	MessageStream out;
	core::String name;
};
using RemoteClients = core::DynamicArray<RemoteClient>;

class NetworkListener {
public:
	virtual ~NetworkListener() {
	}

	virtual void onConnect(RemoteClient *) {
	}
	virtual void onDisconnect(RemoteClient *) {
	}
};

class ServerNetwork : public core::DeltaFrameSeconds {
protected:
	NetworkImpl *_impl;

	double _pingSeconds = 0.0;
	ProtocolHandlerRegistry _protocolRegistry;
	network::NopHandler _nopHandler;
	CommandHandlerServer _commandHandler;
	InitSessionHandler _initSessionHandler;
	SceneStateHandlerServer _sceneStateHandler;
	BroadcastHandler _broadcastHandler;
	core::VarPtr _maxClients;

	RemoteClients _clients;

	using Listeners = core::DynamicArray<NetworkListener *>;
	Listeners _listeners;

	bool updateClient(RemoteClient &client);
	bool sendToClient(RemoteClient &client, ProtocolMessage &msg);

public:
	ServerNetwork(Server *server);
	virtual ~ServerNetwork();

	bool start(uint16_t port = 10001u, const core::String &iface = "0.0.0.0");
	void stop();
	bool isRunning() const;
	void construct() override;
	bool init() override;
	void shutdown() override;
	void update(double nowSeconds);
	void disconnect(ClientId clientId);

	const RemoteClients &clients() const;

	void addListener(NetworkListener *listener);
	void removeListener(NetworkListener *listener);

	size_t clientCount() const;
	RemoteClient *client(ClientId clientId);
	/**
	 * @return @c false if there are no clients
	 */
	bool broadcast(ProtocolMessage &msg, ClientId except = 0xFF);
	bool sendToClient(ClientId clientId, ProtocolMessage &msg);
};

inline RemoteClient *ServerNetwork::client(ClientId clientId) {
	if (clientId >= (ClientId)_clients.size()) {
		return nullptr;
	}
	return &_clients[clientId];
}

inline const RemoteClients &ServerNetwork::clients() const {
	return _clients;
}

inline size_t ServerNetwork::clientCount() const {
	return clients().size();
}

} // namespace network
} // namespace voxedit
