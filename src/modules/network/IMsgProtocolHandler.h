/**
 * @file
 */

#pragma once

#include "IProtocolHandler.h"
#include "core/Log.h"
#include "core/Common.h"

namespace network {

template<class MSGTYPE, class ATTACHMENTTYPE>
class IMsgProtocolHandler: public IProtocolHandler {
private:
	bool _needsAttachment;
	const char *_msgType;
public:
	/**
	 * @param[in] needsAttachment If this is true, the peer must have set the
	 * attachment set already.
	 */
	IMsgProtocolHandler(bool needsAttachment = false, const char *msgType = "") :
			_needsAttachment(needsAttachment), _msgType(msgType) {
	}

	virtual ~IMsgProtocolHandler() {
	}

	virtual void execute(ATTACHMENTTYPE* attachment, const MSGTYPE* message) = 0;

	virtual void execute(ENetPeer* peer, const void* message) override {
		auto* attachment = getAttachment<ATTACHMENTTYPE>(peer);
		if (_needsAttachment && attachment == nullptr) {
			Log::error("No attachment yet for a message that needs one: %s", _msgType);
			return;
		}
		const auto* msg = getMsg<MSGTYPE>(message);
		execute(attachment, msg);
	}
};

}

#define MSGPROTOHANDLER(attachmentType, msgType, attachmentNeeded) \
struct msgType##Handler: public network::IMsgProtocolHandler<msgType, attachmentType> { \
	msgType##Handler() : network::IMsgProtocolHandler<msgType, attachmentType>(true, CORE_STRINGIFY(msgType)) {} \
	void execute(attachmentType* attachment, const msgType* message) override; \
}
