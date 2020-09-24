/**
 * @file
 */

#include "AIMessages_generated.h"
#include "AINetwork.h"
#include "network/IProtocolHandler.h"
#include "core/Assert.h"
#include "core/Trace.h"
#include "core/Log.h"

namespace network {

AINetwork::AINetwork(const ProtocolHandlerRegistryPtr& protocolHandlerRegistry, const core::EventBusPtr& eventBus) :
		Super(protocolHandlerRegistry, eventBus) {
}

bool AINetwork::packetReceived(ENetEvent& event) {
	flatbuffers::Verifier v(event.packet->data, event.packet->dataLength);

	if (!ai::VerifyMessageBuffer(v)) {
		Log::error("Illegal server packet received with length: %i", (int)event.packet->dataLength);
		return false;
	}
	const ai::Message *req = ai::GetMessage(event.packet->data);
	ai::MsgType type = req->data_type();
	const char *typeName = ai::EnumNameMsgType(type);
	const ProtocolHandlerPtr& handler = _protocolHandlerRegistry->getHandler(type);
	if (!handler) {
		Log::error("No handler for ai msg type %s", typeName);
		return false;
	}
	Log::debug("Received %s (size: %i)", typeName, (int)event.packet->dataLength);
	handler->executeWithRaw(event.peer, req->data(), (const uint8_t*)event.packet->data, event.packet->dataLength);
	return true;
}

}
