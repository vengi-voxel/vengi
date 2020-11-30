/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "network/Network.h"

#include <flatbuffers/flatbuffers.h>

namespace backend {

class SignupValidateHandler: public network::IProtocolHandler {
private:
	static constexpr auto logid = Log::logid("SignupValidateHandler");
	network::NetworkPtr _network;
	persistence::DBHandlerPtr _dbHandler;
	network::ServerMessageSenderPtr _messageSender;
	flatbuffers::FlatBufferBuilder _validationFailed;
	flatbuffers::FlatBufferBuilder _validationSucessful;
	void sendValidationFailed(ENetPeer* peer);
	void sendValidationSuccessful(ENetPeer* peer);

public:
	SignupValidateHandler(const network::NetworkPtr& network, const persistence::DBHandlerPtr& dbHandler, const network::ServerMessageSenderPtr &messageSender);

	void executeWithRaw(ENetPeer* peer, const void* message, const uint8_t* rawData, size_t rawDataLength) override;
};

}
