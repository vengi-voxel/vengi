/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "ClientMessages_generated.h"
#include "network/IMsgProtocolHandler.h"
#include "backend/entity/User.h"

using namespace network;

namespace backend {

template<class MSGTYPE>
class IUserProtocolHandler: public network::IMsgProtocolHandler<MSGTYPE, User> {
public:
	IUserProtocolHandler(const char *msgType) :
		network::IMsgProtocolHandler<MSGTYPE, User>(true, msgType) {
	}

	virtual ~IUserProtocolHandler() {
	}
};

#define USERPROTOHANDLER(msgType) \
struct msgType##Handler: public IUserProtocolHandler<msgType> { \
	msgType##Handler() : IUserProtocolHandler<msgType>(#msgType) {} \
	void executeWithRaw(User* attachment, const msgType* message, const uint8_t* rawData, size_t rawDataLength) override; \
}

#define USERPROTOHANDLERIMPL(msgType) \
USERPROTOHANDLER(msgType); \
inline void msgType##Handler::executeWithRaw(User* user, const msgType* message, const uint8_t* rawData, size_t rawDataLength)

}
