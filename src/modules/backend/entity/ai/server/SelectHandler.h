/**
 * @file
 */
#pragma once

#include "network/IMsgProtocolHandler.h"
#include "AIMessages_generated.h"

namespace backend {

class Server;

class SelectHandler: public network::IMsgProtocolHandler<ai::Select, void> {
private:
	Server& _server;
public:
	explicit SelectHandler(Server& server);

	void executeWithRaw(void* attachment, const ai::Select* message, const uint8_t* rawData, size_t rawDataSize) override;
};

}
