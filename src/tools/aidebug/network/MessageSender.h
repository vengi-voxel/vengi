/**
 * @file
 */

#pragma once

#include "AIMessages_generated.h"
#include "AINetwork.h"
#include "core/SharedPtr.h"

namespace network {

using namespace flatbuffers;

/**
 * Create and send messages from the client to the server.
 */
class MessageSender {
private:
	AINetworkPtr _network;
public:
	MessageSender(const AINetworkPtr& network);
	/**
	 * @return @c true if the message was queued for sending.
	 */
	bool sendMessage(FlatBufferBuilder& fbb, ai::MsgType type, Offset<void> data, uint32_t flags = ENET_PACKET_FLAG_RELIABLE);
};

typedef core::SharedPtr<MessageSender> MessageSenderPtr;

}
