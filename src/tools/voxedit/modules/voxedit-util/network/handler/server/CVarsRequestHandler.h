/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/CVarsRequestMessage.h"

namespace voxedit {

class ServerNetwork;

class CVarsRequestHandler : public network::ProtocolTypeHandler<CVarsRequestMessage> {
private:
	ServerNetwork *_network;

public:
	CVarsRequestHandler(ServerNetwork *network);
	void execute(const network::ClientId &clientId, CVarsRequestMessage *msg) override;
};

} // namespace voxedit
