#pragma once

#include "network/Network.h"
#include "network/IMsgProtocolHandler.h"
#include "network/messages/ServerMessages.h"
#include "network/messages/ClientMessages.h"
#include "Client.h"

template<class MSGTYPE>
class IClientProtocolHandler: public network::IMsgProtocolHandler<MSGTYPE, Client> {
public:
	IClientProtocolHandler() :
			network::IMsgProtocolHandler<MSGTYPE, Client>(true) {
	}

	virtual ~IClientProtocolHandler() {
	}

	virtual void execute(Client* attachment, const MSGTYPE* message) override = 0;
};

#define CLIENTPROTOHANDLER(msgType) \
struct msgType##Handler: public IClientProtocolHandler<msgType> { \
	msgType##Handler() : IClientProtocolHandler<msgType>() {} \
	void execute(Client* attachment, const msgType* message) override; \
}

#define CLIENTPROTOHANDLERIMPL(msgType) \
CLIENTPROTOHANDLER(msgType); \
inline void msgType##Handler::execute(Client* client, const msgType* message)
