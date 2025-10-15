/**
 * @file
 */

#pragma once

#include "ClientNetwork.h"
#include "core/IComponent.h"
#include "memento/IMementoStateListener.h"

namespace voxedit {

class SceneManager;

namespace network {

class Client : public core::IComponent, public memento::IMementoStateListener {
protected:
	SceneManager *_sceneMgr = nullptr;
	network::ClientNetwork _network;
	bool _locked = false;

public:
	Client(SceneManager *sceneMgr) : _sceneMgr(sceneMgr), _network(sceneMgr) {
	}
	virtual ~Client();

	network::ClientNetwork &network() {
		return _network;
	}

	void construct() override;
	bool init() override;
	void shutdown() override;
	bool isConnected() const;
	bool connect(const core::String &hostname, uint16_t port = 10001, bool localServer = false);
	void disconnect();
	void update(double nowSeconds);

	void executeCommand(const core::String &command);
	void sendSceneState();

	void lockListener();
	void unlockListener();
	void onMementoStateAdded(const memento::MementoState &state) override;
	void onMementoStateSkipped(const memento::MementoState &state) override;
};

} // namespace network
} // namespace voxedit
