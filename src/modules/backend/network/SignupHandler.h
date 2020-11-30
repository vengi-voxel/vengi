/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "network/Network.h"

namespace backend {

class SignupHandler: public network::IProtocolHandler {
private:
	static constexpr auto logid = Log::logid("SignupHandler");
	persistence::DBHandlerPtr _dbHandler;

	void sendTokenMail(const core::String& email, const core::String& token);

public:
	SignupHandler(const persistence::DBHandlerPtr& dbHandler);

	void executeWithRaw(ENetPeer* peer, const void* message, const uint8_t* rawData, size_t rawDataLength) override;
};

}
