/**
 * @file
 */
#pragma once

#include "ProtocolMessage.h"
#include "core/Log.h"
#include "core/SharedPtr.h"

namespace network {

typedef uint8_t ClientId;

/**
 * @brief Interface for the execution of assigned IProtocolMessage
 *
 * @note Register handler implementations at the @c ProtocolHandlerRegistry
 */
class ProtocolHandler {
public:
	virtual ~ProtocolHandler() {
	}

	virtual void execute(const ClientId &clientId, ProtocolMessage &message) = 0;
};

template<class T>
class ProtocolTypeHandler : public ProtocolHandler {
public:
	virtual ~ProtocolTypeHandler() {
	}

	void execute(const ClientId &clientId, ProtocolMessage &message) override {
		T *msg = (T *)&message;
		execute(clientId, msg);
	}

	virtual void execute(const ClientId &clientId, T *message) = 0;
};

class NopHandler : public ProtocolHandler {
public:
	void execute(const ClientId & /*clientId*/, ProtocolMessage &message) override {
		Log::debug("NOP handler called for message ID %d", message.getId());
	}
};

typedef core::SharedPtr<ProtocolHandler> ProtocolHandlerPtr;

} // namespace network
