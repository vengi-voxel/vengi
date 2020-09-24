/**
 * @file
 */
#pragma once

#include "network/IMsgProtocolHandler.h"
#include "AIMessages_generated.h"

namespace backend {

class Server;

class ResetHandler: public network::IMsgProtocolHandler<ai::Reset, void> {
private:
	Server& _server;
public:
	explicit ResetHandler(Server& server);

	void executeWithRaw(void* attachment, const ai::Reset* message, const uint8_t* rawData, size_t rawDataSize) override;
};

}
