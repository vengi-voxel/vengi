/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"

namespace ai {

/**
 * @brief Message for the remote debugging interface
 *
 * If this is received on the server side, it will pause the execution of
 * the behaviour tree for all ai controlled entities. You can then step
 * the execution of all those entities by sending a @c AIStepMessage
 *
 * The server is sending the @AIPauseMessage back to the clients so they know
 * whether it worked or not.
 */
class AIPauseMessage: public IProtocolMessage {
private:
	bool _pause;

public:
	explicit AIPauseMessage(bool pause) :
			IProtocolMessage(PROTO_PAUSE),_pause(pause) {
	}

	explicit AIPauseMessage(streamContainer& in) :
			IProtocolMessage(PROTO_PAUSE) {
		_pause = readBool(in);
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		addBool(out, _pause);
	}

	inline bool isPause() const {
		return _pause;
	}
};

}
