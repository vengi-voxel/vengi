/**
 * @file
 */
#pragma once

#include "network/IMsgProtocolHandler.h"
#include "AIMessages_generated.h"

namespace backend {

class Server;

class StepHandler: public network::IMsgProtocolHandler<ai::Step, void> {
private:
	Server& _server;
public:
	explicit StepHandler(Server& server);

	void executeWithRaw(void* attachment, const ai::Step* message, const uint8_t* rawData, size_t rawDataSize) override;
};

}
