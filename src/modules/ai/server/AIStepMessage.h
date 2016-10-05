/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"

namespace ai {

/**
 * @brief Perform one step if the ai controlled entities are in paused mode
 *
 * Also see @c AIPauseMessage
 */
class AIStepMessage: public IProtocolMessage {
private:
	int64_t _millis;

public:
	explicit AIStepMessage(int64_t millis) :
			IProtocolMessage(PROTO_STEP) {
		_millis = millis;
	}

	explicit AIStepMessage(streamContainer& in) :
			IProtocolMessage(PROTO_STEP) {
		_millis = readLong(in);
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		addLong(out, _millis);
	}

	inline int64_t getStepMillis() const {
		return _millis;
	}
};

}
