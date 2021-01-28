/**
 * @file
 */

#include "ServerMessages_generated.h"
#include "ClientNetwork.h"
#include "core/Log.h"

namespace network {

ClientNetwork::ClientNetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus) :
		Super(protocolHandlerRegistry, eventBus) {
}

bool ClientNetwork::packetReceived(ENetEvent& event) {
	flatbuffers::Verifier v(event.packet->data, event.packet->dataLength);

	if (!VerifyServerMessageBuffer(v)) {
		Log::error("Illegal server packet received with length: %i", (int)event.packet->dataLength);
		return false;
	}
	const ServerMessage *req = GetServerMessage(event.packet->data);
	ServerMsgType type = req->data_type();
	const char *typeName = EnumNameServerMsgType(type);
	ProtocolHandlerPtr handler = _protocolHandlerRegistry->getHandler(type);
	if (!handler) {
		Log::error("No handler for server msg type %s", typeName);
		return false;
	}
	Log::debug("Received %s", typeName);
	handler->executeWithRaw(event.peer, req->data(), (const uint8_t*)event.packet->data, event.packet->dataLength);
	return true;
}

}
