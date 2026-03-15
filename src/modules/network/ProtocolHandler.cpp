/**
 * @file
 */

#include "network/ProtocolHandler.h"
#include "network/ProtocolMessage.h"

namespace network {

void NopHandler::execute(const ClientId & /*clientId*/, ProtocolMessage &message) {
	Log::debug("NOP handler called for message ID %d", message.getId());
}

} // namespace network
