/**
 * @file
 */
#pragma once

#include "network/IMsgProtocolHandler.h"
#include "AIMessages_generated.h"

namespace backend {

class Server;

class PauseHandler: public network::IMsgProtocolHandler<ai::Pause, void> {
private:
	Server& _server;
public:
	explicit PauseHandler(Server& server);

	void executeWithRaw(void* attachment, const ai::Pause* message, const uint8_t* rawData, size_t rawDataSize) override;
};

}
