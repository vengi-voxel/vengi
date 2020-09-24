/**
 * @file
 */

#include "AIMessages_generated.h"
#include "AIServerNetwork.h"
#include "core/Trace.h"
#include "core/Log.h"

namespace network {

AIServerNetwork::AIServerNetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry,
		const core::EventBusPtr& eventBus, const metric::MetricPtr& metric) :
		Super(protocolHandlerRegistry, eventBus, metric) {
}

bool AIServerNetwork::packetReceived(ENetEvent& event) {
	flatbuffers::Verifier v(event.packet->data, event.packet->dataLength);

	if (!ai::VerifyMessageBuffer(v)) {
		Log::error("Illegal ai packet received with length: %i", (int)event.packet->dataLength);
		return false;
	}
	const ai::Message *req = ai::GetMessage(event.packet->data);
	ai::MsgType type = req->data_type();
	const char *clientMsgType = ai::EnumNameMsgType(type);
	ProtocolHandlerPtr handler = _protocolHandlerRegistry->getHandler(type);
	if (!handler) {
		Log::error("No handler for ai msg type %s", clientMsgType);
		return false;
	}
	const metric::TagMap& tags  {{"direction", "in"}, {"type", clientMsgType}};
	_metric->count("network_packet_count", 1, tags);
	_metric->count("network_packet_size", (int)event.packet->dataLength, tags);

	Log::debug("Received %s", clientMsgType);
	handler->executeWithRaw(event.peer, req->data(), (const uint8_t*)event.packet->data, event.packet->dataLength);
	return true;
}

}
