/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"

namespace ai {

/**
 * @brief Message for the remote debugging interface
 *
 * Allows to select a particular subset of entities to receive the state for
 * by handling the @c AINamesMessage
 */
class AIChangeMessage: public IProtocolMessage {
private:
	const core::String _name;

public:
	explicit AIChangeMessage(const core::String& name) :
			IProtocolMessage(PROTO_CHANGE), _name(name) {
	}

	explicit AIChangeMessage(streamContainer& in) :
			IProtocolMessage(PROTO_CHANGE), _name(readString(in)) {
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		addString(out, _name);
	}

	inline const core::String& getName() const {
		return _name;
	}
};

}
