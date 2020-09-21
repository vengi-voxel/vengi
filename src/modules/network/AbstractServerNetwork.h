/**
 * @file
 */

#pragma once

#include "network/Network.h"
#include "metric/Metric.h"

namespace network {

class AbstractServerNetwork : public Network {
private:
	using Super = Network;
protected:
	ENetHost* _server = nullptr;
	metric::MetricPtr _metric;
public:
	AbstractServerNetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry,
			const core::EventBusPtr& eventBus, const metric::MetricPtr& metric);

	bool bind(uint16_t port, const core::String& hostname = "", int maxPeers = 1024, int maxChannels = 1);

	bool broadcast(ENetPacket* packet, int channel = 0);

	void update();
	void shutdown() override;
};

}
