/**
 * @file
 */

#pragma once

#include "IProtocolHandler.h"
#include "core/Log.h"

namespace network {

template<class MSGTYPE, class ATTACHMENTTYPE>
class IMsgProtocolHandler: public IProtocolHandler {
private:
	bool _needsAttachment;
public:
	/**
	 * @param[in] needsAttachment If this is true, the peer must have set the
	 * attachment set already.
	 */
	IMsgProtocolHandler(bool needsAttachment = false) :
			_needsAttachment(needsAttachment) {
	}

	virtual ~IMsgProtocolHandler() {
	}

	virtual void execute(ATTACHMENTTYPE* attachment, const MSGTYPE* message) = 0;

	virtual void execute(ENetPeer* peer, const void* message) override {
		auto* attachment = getAttachment<ATTACHMENTTYPE>(peer);
		if (_needsAttachment && attachment == nullptr) {
			::Log::error("No attachment yet for a message that needs one");
			return;
		}
		const auto* msg = getMsg<MSGTYPE>(message);
		execute(attachment, msg);
	}
};

}

#define MSGPROTOHANDLER(attachmentType, msgType, attachmentNeeded) \
struct msgType##Handler: public network::IMsgProtocolHandler<msgType, attachmentType> { \
	msgType##Handler() : network::IMsgProtocolHandler<msgType, attachmentType>(true) {} \
	void execute(attachmentType* attachment, const msgType* message) override; \
}
