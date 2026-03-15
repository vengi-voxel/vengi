/**
 * @file
 */

#pragma once

namespace voxedit {

struct RemoteClient;

class NetworkListener {
public:
	virtual ~NetworkListener() {
	}

	virtual void onConnect(RemoteClient *) {
	}
	virtual void onDisconnect(RemoteClient *) {
	}
};

} // namespace voxedit
