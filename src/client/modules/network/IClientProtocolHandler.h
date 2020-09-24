/**
 * @file
 */

#pragma once

#include "ServerMessages_generated.h"
#include "ClientMessages_generated.h"
#include "network/Network.h"
#include "network/IMsgProtocolHandler.h"
#include "../Client.h"

using namespace network;

template<class MSGTYPE>
class IClientProtocolHandler: public network::IMsgProtocolHandler<MSGTYPE, Client> {
public:
	IClientProtocolHandler() :
			network::IMsgProtocolHandler<MSGTYPE, Client>(true) {
	}

	virtual ~IClientProtocolHandler() {
	}
};

#define CLIENTPROTOHANDLER(msgType) \
struct msgType##Handler: public IClientProtocolHandler<msgType> { \
	msgType##Handler() : IClientProtocolHandler<msgType>() {} \
	void executeWithRaw(Client* attachment, const msgType* message, const uint8_t* rawData, size_t rawDataSize) override; \
}

#define CLIENTPROTOHANDLERIMPL(msgType) \
CLIENTPROTOHANDLER(msgType); \
inline void msgType##Handler::executeWithRaw(Client* client, const msgType* message, const uint8_t* rawData, size_t rawDataSize)
