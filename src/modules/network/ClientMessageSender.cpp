/**
 * @file
 */

#include "ClientMessageSender.h"
#include "core/Log.h"
#include "core/Common.h"

namespace network {

inline ENetPacket* createClientPacket(FlatBufferBuilder& fbb, ClientMsgType type, Offset<void> data, uint32_t flags) {
	auto msg = CreateClientMessage(fbb, type, data);
	FinishClientMessageBuffer(fbb, msg);
	ENetPacket* packet = enet_packet_create(fbb.GetBufferPointer(), fbb.GetSize(), flags);
	Log::trace("Create client package: %s - size %ui", EnumNameClientMsgType(type), fbb.GetSize());
	return packet;
}

ClientMessageSender::ClientMessageSender(const ClientNetworkPtr& network) :
		_network(network) {
}

bool ClientMessageSender::sendClientMessage(FlatBufferBuilder& fbb, ClientMsgType type, Offset<void> data, uint32_t flags) {
	const bool retVal = _network->sendMessage(createClientPacket(fbb, type, data, flags));
	fbb.Clear();
	return retVal;
}

}
