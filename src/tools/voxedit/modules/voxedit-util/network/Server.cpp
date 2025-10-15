/**
 * @file
 */

#include "Server.h"
#include "core/Log.h"
#include "voxedit-util/network/ProtocolVersion.h"
#include "voxedit-util/network/ServerNetwork.h"
#include "voxedit-util/network/protocol/SceneStateMessage.h"
#include "voxedit-util/network/protocol/SceneStateRequestMessage.h"

namespace voxedit {
namespace network {

Server::Server() : _network(this) {
	_network.addListener(this);
}

Server::~Server() {
	shutdown();
}

bool Server::shouldRequestClientState(bool localServer) const {
	if (localServer) {
		return false;
	}
	if (_sceneGraph == nullptr || !_sceneGraph->empty()) {
		return false;
	}
	// the headless server is a client, itself. So the second client is basically the one we request the state from.
	return _network.clientCount() == 2;
}

bool Server::shouldSendClientState(bool localServer) const {
	if (localServer) {
		return false;
	}
	if (_sceneGraph == nullptr) {
		return false;
	}
	return !_sceneGraph->empty();
}

bool Server::initSession(const ClientId &clientId, uint32_t protocolVersion, const core::String &applicationVersion,
						 const core::String &username, const core::String &password, bool localServer) {
	if (protocolVersion != PROTOCOL_VERSION) {
		Log::error("Client %u has incompatible protocol version %u (expected 1)", clientId, protocolVersion);
		return false;
	}

	const core::VarPtr &expectedPassword = core::Var::getSafe(cfg::VoxEditNetPassword);
	if (expectedPassword->strVal() != password) {
		Log::error("Client %u provided invalid password", clientId);
		return false;
	}

	Log::info("Client %u connected with application version %s and username %s", clientId, applicationVersion.c_str(),
			  username.c_str());

	if (RemoteClient *c = _network.client(clientId)) {
		c->name = username;
	} else {
		Log::error("Client %u not found", clientId);
		return false;
	}

	if (shouldRequestClientState(localServer)) {
		Log::info("Requesting scene state from client %u", clientId);
		SceneStateRequestMessage msg;
		if (!_network.sendToClient(clientId, msg)) {
			Log::error("Failed to request scene state from client %u", clientId);
			return false;
		}
	} else if (shouldSendClientState(localServer)) {
		Log::info("Sending scene state to client %u", clientId);
		SceneStateMessage msg(*_sceneGraph);
		if (!_network.sendToClient(clientId, msg)) {
			Log::error("Failed to send scene state to client %u", clientId);
			return false;
		}
	} else if (!localServer) {
		Log::warn("No request nor send of the state for client %u", clientId);
	}

	return true;
}

void Server::disconnect(const ClientId &clientId) {
	_network.disconnect(clientId);
}

void Server::onConnect(RemoteClient *client) {
	Log::info("remote client connect (%i)", (int)_network.clientCount());
}

void Server::onDisconnect(RemoteClient *client) {
	Log::info("remote client disconnect (%i): %s", (int)_network.clientCount(), client->name.c_str());
}

void Server::construct() {
	_network.construct();
}

bool Server::init() {
	return _network.init();
}

bool Server::start(uint16_t port, const core::String &iface) {
	return _network.start(port, iface);
}

void Server::stop() {
	_network.stop();
}

bool Server::isRunning() const {
	return _network.isRunning();
}

void Server::update(double nowSeconds) {
	_network.update(nowSeconds);
}

void Server::shutdown() {
	_network.removeListener(this);
	_network.shutdown();
}

const RemoteClients &Server::clients() const {
	return _network.clients();
}

} // namespace network
} // namespace voxedit
