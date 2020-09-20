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
	ProtocolHandlerPtr handler = _protocolHandlerRegistry->getHandler(EnumNameServerMsgType(type));
	if (!handler) {
		Log::error("No handler for server msg type %s", EnumNameServerMsgType(type));
		return false;
	}
	Log::debug("Received %s", EnumNameServerMsgType(type));
	handler->execute(event.peer, reinterpret_cast<const flatbuffers::Table*>(req->data()));
	return true;
}

}
