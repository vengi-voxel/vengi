/**
 * @file
 */
#pragma once

#include "network/IMsgProtocolHandler.h"
#include "AIMessages_generated.h"

namespace backend {

class Server;

class ChangeHandler: public network::IMsgProtocolHandler<ai::ChangeZone, void> {
private:
	Server& _server;
public:
	explicit ChangeHandler(Server& server);

	void executeWithRaw(void* attachment, const ai::ChangeZone* message, const uint8_t* rawData, size_t rawDataSize) override;
};

}
