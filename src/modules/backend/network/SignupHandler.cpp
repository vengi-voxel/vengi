/**
 * @file
 */

#include "ClientMessages_generated.h"
#include "SignupHandler.h"
#include "UserModel.h"
#include "core/Password.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "math/Random.h"
#include "util/EMailValidator.h"
#include "persistence/DBHandler.h"
#include "SignupModel.h"

namespace backend {

SignupHandler::SignupHandler(const persistence::DBHandlerPtr& dbHandler) : _dbHandler(dbHandler) {
}

static core::String generateSignupToken(unsigned int seed) {
	math::Random random(seed);
	const int rnd = random.random(10000, 99999);
	return core::string::toString(rnd);
}

void SignupHandler::sendTokenMail(const core::String& email, const core::String& token) {
	Log::info("Send token mail to %s", email.c_str());
	// TODO: send mail https://tools.ietf.org/html/rfc5321
	// send mail to a mail relay that listens on localhost without tls for local connections. The TLS is handled in the relay
	// this would allow us to send a mail with a few lines of code without all the hassle....
}

void SignupHandler::executeWithRaw(ENetPeer* peer, const void* raw, const uint8_t* rawData, size_t rawDataLength) {
	const auto* message = getMsg<network::Signup>(raw);

	const core::String email(message->email()->c_str());
	if (!util::isValidEmail(email)) {
		Log::debug(logid, "Invalid email given: '%s', %c", email.c_str(), email[0]);
		return;
	}

	const core::String password(message->password()->c_str());
	if (password.empty()) {
		Log::debug("Abort signup. No password was given.");
		return;
	}

	db::DBConditionUserModelEmail userEmailCond(email);
	const int count = _dbHandler->count(db::UserModel(), userEmailCond);
	if (count != 0) {
		Log::info("Abort signup. Account for %s already exists", email.c_str());
		// TODO: allow to claim via token
		return;
	}

	db::UserModel userModel;
	userModel.setEmail(email);
	userModel.setValidated(false);
	userModel.setName(email);
	userModel.setPassword(core::pwhash(password, "TODO"));
	if (!_dbHandler->insert(userModel)) {
		Log::error("Failed to register user for %s", email.c_str());
		return;
	}

	const core::String& token = generateSignupToken(((uint32_t)(intptr_t)this) + peer->connectID);
	Log::info("User registered with id %i: %s", (int)userModel.id(), email.c_str());
	db::SignupModel model;
	model.setUserid(userModel.id());
	model.setToken(token);
	if (!_dbHandler->insert(model)) {
		Log::info(logid, "Could not create signup request for %s.", email.c_str());
		return;
	}

	sendTokenMail(email, token);
}

}
