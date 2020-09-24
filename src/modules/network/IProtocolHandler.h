/**
 * @file
 */

#pragma once

extern "C" {
#include <enet/enet.h>
}
#include <memory>
#include "core/Common.h"
#include <stdint.h>

namespace network {

/**
 * @brief Interface for the execution of assigned IProtocolMessage
 *
 * @note Register handler implementations at the @c ProtocolHandlerRegistry
 */
class IProtocolHandler {
protected:
	template<class ATTACHMENTTYPE>
	inline ATTACHMENTTYPE* getAttachment(ENetPeer* peer) const {
		return static_cast<ATTACHMENTTYPE*>(peer->data);
	}

	template<class MSGTYPE>
	inline const MSGTYPE* getMsg(const void *data) const {
		const MSGTYPE* msg = static_cast<const MSGTYPE*>(data);
		return msg;
	}

public:
	virtual ~IProtocolHandler() {
	}

	virtual void executeWithRaw(ENetPeer* peer, const void* message, const uint8_t* /*rawData*/, size_t /*rawDataSize*/) = 0;
};

class NopHandler : public IProtocolHandler {
};

typedef std::shared_ptr<IProtocolHandler> ProtocolHandlerPtr;

}
