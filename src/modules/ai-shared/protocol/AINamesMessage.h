/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace ai {

/**
 * @brief Message for the remote debugging interface
 *
 * Get a list of all potential subsets that can be selected by @c AIChangeMessage
 */
class AINamesMessage: public IProtocolMessage {
private:
	core::DynamicArray<core::String> _names;
	const core::DynamicArray<core::String>* _namesPtr;

public:
	explicit AINamesMessage(const core::DynamicArray<core::String>& names) :
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
		const size_t size = _namesPtr->size();
		addInt(out, static_cast<int>(size));
		for (size_t i = 0U; i < size; ++i) {
			addString(out, (*_namesPtr)[i]);
		}
	}

	inline const core::DynamicArray<core::String>& getNames() const {
		if (_namesPtr)
			return *_namesPtr;
		return _names;
	}
};

}
