/**
 * @file
 */
#pragma once

#include "core/DeltaFrameSeconds.h"
#include "core/collection/DynamicArray.h"
#include "handler/server/InitSessionHandler.h"
#include "handler/server/SceneStateHandlerServer.h"
#include "network/ProtocolHandler.h"
#include "network/ProtocolHandlerRegistry.h"
#include "network/SocketId.h"
#include "voxedit-util/network/handler/server/BroadcastHandler.h"
#include "voxedit-util/network/handler/server/CVarsRequestHandler.h"
#include "voxedit-util/network/handler/server/CommandHandlerServer.h"
#include "voxedit-util/network/handler/server/CommandsRequestHandler.h"
#include "voxedit-util/network/handler/server/LuaScriptCreateHandler.h"
#include "voxedit-util/network/handler/server/LuaScriptsRequestHandler.h"

namespace network {
struct NetworkImpl;
}

namespace voxelgenerator {
class LUAApi;
}

namespace voxedit {

class ProtocolMessage;

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
	network::NetworkImpl *_impl;

	double _pingSeconds = 0.0;
	network::ProtocolHandlerRegistry _protocolRegistry;
	network::NopHandler _nopHandler;
	CommandHandlerServer _commandHandler;
	InitSessionHandler _initSessionHandler;
	SceneStateHandlerServer _sceneStateHandler;
	BroadcastHandler _broadcastHandler;
	LuaScriptsRequestHandler _luaScriptsRequestHandler;
	LuaScriptCreateHandler _luaScriptCreateHandler;
	CVarsRequestHandler _cvarsRequestHandler;
	CommandsRequestHandler _commandsRequestHandler;
	core::VarPtr _maxClients;

	RemoteClients _clients;
	core::DynamicArray<network::ClientId> _pendingDisconnects;

	using Listeners = core::DynamicArray<NetworkListener *>;
	Listeners _listeners;

	bool updateClient(RemoteClient &client);
	bool sendToClient(RemoteClient &client, network::ProtocolMessage &msg);

public:
	ServerNetwork(Server *server, voxelgenerator::LUAApi *luaApi);
	virtual ~ServerNetwork();

	bool start(uint16_t port = 10001u, const core::String &iface = "0.0.0.0");
	void stop();
	bool isRunning() const;
	void construct() override;
	bool init() override;
	void shutdown() override;
	void update(double nowSeconds);
	void disconnect(network::ClientId clientId);
	void markForDisconnect(network::ClientId clientId);

	const RemoteClients &clients() const;

	void addListener(NetworkListener *listener);
	void removeListener(NetworkListener *listener);

	size_t clientCount() const;
	RemoteClient *client(network::ClientId clientId);
	/**
	 * @return @c false if there are no clients
	 */
	bool broadcast(network::ProtocolMessage &msg, network::ClientId except = 0xFF);
	bool sendToClient(network::ClientId clientId, network::ProtocolMessage &msg);
};

inline RemoteClient *ServerNetwork::client(network::ClientId clientId) {
	if (clientId >= (network::ClientId)_clients.size()) {
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

} // namespace voxedit
