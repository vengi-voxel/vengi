/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"
#include "ai-shared/common/CharacterId.h"

namespace ai {

/**
 * @brief Message for the remote debugging interface
 *
 * Select a particular character to get detailed information about its state
 */
class AISelectMessage: public IProtocolMessage {
private:
	ai::CharacterId _chrId;

public:
	explicit AISelectMessage(ai::CharacterId id) :
			IProtocolMessage(PROTO_SELECT) {
		_chrId = id;
	}

	explicit AISelectMessage(streamContainer& in) :
			IProtocolMessage(PROTO_SELECT) {
		_chrId = readInt(in);
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		addInt(out, _chrId);
	}

	inline const CharacterId& getCharacterId() const {
		return _chrId;
	}
};

}
