/**
 * @file
 */
#pragma once

#include "network/IMsgProtocolHandler.h"
#include "AIMessages_generated.h"

namespace backend {

class Server;

class UpdateNodeHandler: public network::IMsgProtocolHandler<ai::UpdateNode, void> {
private:
	Server& _server;
public:
	explicit UpdateNodeHandler(Server& server);

	void executeWithRaw(void* attachment, const ai::UpdateNode* message, const uint8_t* rawData, size_t rawDataSize) override;
};

}
