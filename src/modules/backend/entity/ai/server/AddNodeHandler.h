/**
 * @file
 */
#pragma once

#include "network/IMsgProtocolHandler.h"
#include "AIMessages_generated.h"

namespace backend {

class Server;

class AddNodeHandler: public network::IMsgProtocolHandler<ai::AddNode, void> {
private:
	Server& _server;
public:
	explicit AddNodeHandler(Server& server);

	void executeWithRaw(void* attachment, const ai::AddNode* message, const uint8_t* rawData, size_t rawDataSize) override;
};

}
