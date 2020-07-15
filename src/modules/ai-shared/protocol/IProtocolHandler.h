/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"
#include <memory>
#include <stdint.h>

namespace ai {

typedef uint8_t ClientId;

/**
 * @brief Interface for the execution of assigned IProtocolMessage
 *
 * @note Register handler implementations at the @c ProtocolHandlerRegistry
 */
class IProtocolHandler {
public:
	virtual ~IProtocolHandler() {
	}

	virtual void execute(const ClientId& clientId, const IProtocolMessage& message) = 0;
};

template<class T>
class ProtocolHandler : public IProtocolHandler {
public:
	virtual ~ProtocolHandler ()
	{
	}

	void execute (const ClientId& clientId, const IProtocolMessage& message) override {
		const T *msg = static_cast<const T*>(&message);
		execute(clientId, msg);
	}

	virtual void execute (const ClientId& clientId, const T* message) = 0;
};

class NopHandler: public IProtocolHandler {
public:
	void execute(const ClientId& /*clientId*/, const IProtocolMessage& /*message*/) override {
	}
};

typedef std::shared_ptr<IProtocolHandler> ProtocolHandlerPtr;

/**
 * @brief Use this deleter for any handler that should not get freed by @c delete.
 */
struct ProtocolHandlerNopDeleter {
	void operator()(IProtocolHandler* /* ptr */) {
	}
};

}
