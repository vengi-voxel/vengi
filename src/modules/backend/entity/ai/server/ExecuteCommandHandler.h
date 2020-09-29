/**
 * @file
 */
#pragma once

#include "network/IMsgProtocolHandler.h"
#include "AIMessages_generated.h"

namespace backend {

class ExecuteCommandHandler: public network::IMsgProtocolHandler<ai::ExecuteCommand, void> {
public:
	void executeWithRaw(void* attachment, const ai::ExecuteCommand* message, const uint8_t* rawData, size_t rawDataSize) override;
};

}
