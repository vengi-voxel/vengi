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

	virtual void executeWithRaw(ATTACHMENTTYPE* attachment, const MSGTYPE* message, const uint8_t* rawData, size_t rawDataSize) = 0;

	virtual void executeWithRaw(ENetPeer* peer, const void* message, const uint8_t* rawData, size_t rawDataSize) override {
		ATTACHMENTTYPE* attachment = getAttachment<ATTACHMENTTYPE>(peer);
		if (_needsAttachment && attachment == nullptr) {
			Log::error("No attachment yet for a message that needs one: %s", _msgType);
			return;
		}
		const MSGTYPE* msg = getMsg<MSGTYPE>(message);
		executeWithRaw(attachment, msg, rawData, rawDataSize);
	}
};

}
