/**
 * @file
 */

#pragma once

#include "network/ProtocolHandler.h"
#include "voxedit-util/network/protocol/LogMessage.h"

namespace voxedit {

/**
 * @brief Handles log messages received from the server and forwards them to the local Log system
 */
class LogMessageHandler : public network::ProtocolTypeHandler<LogMessage> {
public:
	void execute(const network::ClientId &clientId, LogMessage *message) override;
};

} // namespace voxedit
