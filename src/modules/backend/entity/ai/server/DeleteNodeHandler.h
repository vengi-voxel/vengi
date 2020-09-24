/**
 * @file
 */
#pragma once

#include "network/IMsgProtocolHandler.h"
#include "AIMessages_generated.h"

namespace backend {

class Server;

class DeleteNodeHandler: public network::IMsgProtocolHandler<ai::DeleteNode, void> {
private:
	Server& _server;
public:
	explicit DeleteNodeHandler(Server& server);

	void executeWithRaw(void* attachment, const ai::DeleteNode* message, const uint8_t* rawData, size_t rawDataSize) override;
};

}
