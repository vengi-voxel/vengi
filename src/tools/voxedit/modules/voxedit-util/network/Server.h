/**
 * @file
 */
#pragma once

#include "core/IComponent.h"
#include "core/String.h"
#include "network/ProtocolHandler.h"
#include "voxedit-util/network/NetworkListener.h"
#include "voxedit-util/network/RemoteClient.h"

namespace scenegraph {
class SceneGraph;
}

namespace voxelgenerator {
class LUAApi;
}
namespace voxedit {

class ServerNetwork;
class SceneManager;

class Server : public NetworkListener, public core::IComponent {
protected:
	ServerNetwork *_network;
	// the state of the scene graph that the server is broadcasting to the clients
	scenegraph::SceneGraph *_sceneGraph = nullptr;

	void onConnect(RemoteClient *client) override;
	void onDisconnect(RemoteClient *client) override;

	void broadcastClientList();
	bool shouldRequestClientState(bool localServer) const;
	bool shouldSendClientState(bool localServer) const;
public:
	Server(voxelgenerator::LUAApi *luaApi, SceneManager *sceneMgr);
	virtual ~Server();
	void construct() override;
	bool init() override;
	void shutdown() override;

	/**
	 * @return The display name for a connected client - appends #id if another
	 *         client with the same name is also connected.
	 */
	core::String disambiguatedName(const network::ClientId &clientId) const;

	// the scenegraph that is written to
	void setState(scenegraph::SceneGraph *sceneGraph) {
		_sceneGraph = sceneGraph;
	}

	void setSceneGraph(scenegraph::SceneGraph &&sceneGraph);

	ServerNetwork &network() {
		return *_network;
	}

	/**
	 * @return @c false if there are no clients
	 */
	bool broadcast(network::ProtocolMessage &msg, network::ClientId except = 0xFF);

	/**
	 * @brief Start to listen on the specified port and interface
	 */
	bool start(uint16_t port = 10001, const core::String &iface = "0.0.0.0");
	void stop();
	bool isRunning() const;
	void update(double nowSeconds);
	const RemoteClients &clients() const;

	bool initSession(const network::ClientId &clientId, uint32_t protocolVersion, const core::String &applicationVersion,
					 const core::String &username, const core::String &password, bool localServer);
	void disconnect(const network::ClientId &clientId);
	void markForDisconnect(const network::ClientId &clientId);
};


} // namespace voxedit
