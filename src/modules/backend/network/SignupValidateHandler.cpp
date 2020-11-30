/**
 * @file
 */

#include "ClientMessages_generated.h"
#include "SignupValidateHandler.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "math/Random.h"
#include "persistence/DBCondition.h"
#include "persistence/DBHandler.h"
#include "SignupModel.h"
#include "UserModel.h"
#include "backend/network/ServerMessageSender.h"

namespace backend {

SignupValidateHandler::SignupValidateHandler(const network::NetworkPtr& network, const persistence::DBHandlerPtr& dbHandler, const network::ServerMessageSenderPtr &messageSender) :
		_network(network), _dbHandler(dbHandler), _messageSender(messageSender) {
	{
		auto data = network::CreateSignupValidationState(_validationFailed, false);
		auto msg = network::CreateServerMessage(_validationFailed, network::ServerMsgType::SignupValidationState, data.Union());
		network::FinishServerMessageBuffer(_validationFailed, msg);
	}

	{
		auto data = network::CreateSignupValidationState(_validationSucessful, true);
		auto msg = network::CreateServerMessage(_validationSucessful, network::ServerMsgType::SignupValidationState, data.Union());
		network::FinishServerMessageBuffer(_validationSucessful, msg);
	}
}

void SignupValidateHandler::sendValidationSuccessful(ENetPeer* peer) {
	ENetPacket* packet = _messageSender->createServerPacket(network::ServerMsgType::SignupValidationState, _validationSucessful.GetBufferPointer(), _validationSucessful.GetSize(), ENET_PACKET_FLAG_RELIABLE);
	_network->sendMessage(peer, packet);
}

void SignupValidateHandler::sendValidationFailed(ENetPeer* peer) {
	ENetPacket* packet = _messageSender->createServerPacket(network::ServerMsgType::SignupValidationState, _validationSucessful.GetBufferPointer(), _validationSucessful.GetSize(), ENET_PACKET_FLAG_RELIABLE);
	_network->sendMessage(peer, packet);
}

void SignupValidateHandler::executeWithRaw(ENetPeer* peer, const void* raw, const uint8_t* rawData, size_t rawDataLength) {
	const auto* message = getMsg<network::SignupValidate>(raw);

	const core::String email(message->email()->c_str());
	const core::String token(message->token()->c_str());

	db::UserModel userModel;
	if (!_dbHandler->select(userModel, db::DBConditionUserModelEmail(email))) {
		Log::debug(logid, "Could not validate signup request for %s. No user found.", email.c_str());
		sendValidationFailed(peer);
		return;
	}

	db::DBConditionSignupModelUserid condUserId(userModel.id());
	db::DBConditionSignupModelToken condToken(token);
	db::SignupModel model;
	if (!_dbHandler->select(model, persistence::DBConditionMultiple(true, {&condUserId, &condToken}))) {
		Log::debug(logid, "Could not validate signup request for %s.", email.c_str());
		sendValidationFailed(peer);
		return;
	}
	sendValidationSuccessful(peer);
}

}
