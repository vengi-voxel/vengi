/**
 * @file
 */

#pragma once

extern "C" {
#include <enet/enet.h>
}
#include <memory>
#include <sauce/sauce.h>
#include <flatbuffers/flatbuffers.h>
#include "core/Common.h"

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
		// TODO: call verify
		return msg;
	}

public:
	virtual ~IProtocolHandler() {
	}

	virtual void execute(ENetPeer* peer, const void* message) = 0;
};

typedef std::shared_ptr<IProtocolHandler> ProtocolHandlerPtr;

}
