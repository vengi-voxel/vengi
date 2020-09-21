/**
 * @file
 */

#include "AbstractServerNetwork.h"
#include "core/Trace.h"
#include "core/Log.h"

namespace network {

AbstractServerNetwork::AbstractServerNetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry,
		const core::EventBusPtr& eventBus, const metric::MetricPtr& metric) :
		Super(protocolHandlerRegistry, eventBus), _metric(metric) {
}

bool AbstractServerNetwork::bind(uint16_t port, const core::String& hostname, int maxPeers, int maxChannels) {
	if (_server) {
		Log::error("There is already a server socket opened");
		return false;
	}
	if (maxPeers <= 0) {
		Log::error("maxpeers must be greater than 0");
		return false;
	}
	if (maxChannels <= 0) {
		Log::error("maxchannels must be greater than 0");
		return false;
	}
	ENetAddress address;
	if (hostname.empty()) {
		Log::info("Bind to any host interface");
		address.host = ENET_HOST_ANY;
	} else {
		Log::info("Bind to host interface: %s", hostname.c_str());
		enet_address_set_host(&address, hostname.c_str());
	}
	address.port = port;
	_server = enet_host_create(
			&address,
			maxPeers,
			maxChannels,
			0, /* assume any amount of incoming bandwidth */
			0 /* assume any amount of outgoing bandwidth */
			);
	if (_server == nullptr) {
		Log::error("Failed to create host");
		return false;
	}
	enet_host_compress_with_range_coder(_server);
	return true;
}

bool AbstractServerNetwork::broadcast(ENetPacket* packet, int channel) {
	if (_server == nullptr) {
		return false;
	}
	if (packet == nullptr) {
		return false;
	}
	Log::debug("Broadcasting a message on channel %i", channel);
	enet_host_broadcast(_server, channel, packet);
	return true;
}

void AbstractServerNetwork::shutdown() {
	if (_server != nullptr) {
		enet_host_flush(_server);
		enet_host_destroy(_server);
	}
	_server = nullptr;
	Super::shutdown();
}

void AbstractServerNetwork::update() {
	core_trace_scoped(Network);
	updateHost(_server);
}

}
