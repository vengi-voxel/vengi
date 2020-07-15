/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"

namespace ai {

/**
 * @brief Message for the remote debugging interface
 *
 * Get a list of all potential subsets that can be selected by @c AIChangeMessage
 */
class AINamesMessage: public IProtocolMessage {
private:
	std::vector<core::String> _names;
	const std::vector<core::String>* _namesPtr;

public:
	explicit AINamesMessage(const std::vector<core::String>& names) :
			IProtocolMessage(PROTO_NAMES), _namesPtr(&names) {
	}

	explicit AINamesMessage(streamContainer& in) :
			IProtocolMessage(PROTO_NAMES), _namesPtr(nullptr) {
		const int size = readInt(in);
		for (int i = 0; i < size; ++i) {
			_names.push_back(readString(in));
		}
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		const std::size_t size = _namesPtr->size();
		addInt(out, static_cast<int>(size));
		for (std::size_t i = 0U; i < size; ++i) {
			addString(out, (*_namesPtr)[i]);
		}
	}

	inline const std::vector<core::String>& getNames() const {
		if (_namesPtr)
			return *_namesPtr;
		return _names;
	}
};

}
