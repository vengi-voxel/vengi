/**
 * @file
 */
#pragma once

#include "core/SharedPtr.h"

namespace network {

typedef uint8_t ClientId;
class ProtocolMessage;

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
	void execute(const ClientId & /*clientId*/, ProtocolMessage &message) override;
};

typedef core::SharedPtr<ProtocolHandler> ProtocolHandlerPtr;

} // namespace network
