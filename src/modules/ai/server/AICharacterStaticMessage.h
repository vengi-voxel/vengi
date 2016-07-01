#pragma once

#include "AIStubTypes.h"
#include <vector>

namespace ai {

class AICharacterStaticMessage : public IProtocolMessage {
private:
	CharacterId _chrId;
	const std::vector<AIStateNodeStatic>* _nodeStaticDataPtr;
	std::vector<AIStateNodeStatic> _nodeStaticData;
public:
	/**
	 * Make sure that none of the given references is destroyed, for performance reasons we are only storing the
	 * pointers to those instances in this class. So they need to stay valid until they are serialized.
	 */
	AICharacterStaticMessage(const CharacterId& id, const std::vector<AIStateNodeStatic>& nodeStaticData) :
			IProtocolMessage(PROTO_CHARACTER_STATIC), _chrId(id), _nodeStaticDataPtr(&nodeStaticData) {
	}

	explicit AICharacterStaticMessage(streamContainer& in) :
			IProtocolMessage(PROTO_CHARACTER_STATIC), _nodeStaticDataPtr(nullptr) {
		_chrId = readInt(in);
		const std::size_t size = readInt(in);
		_nodeStaticData.reserve(size);
		for (std::size_t i = 0; i < size; ++i) {
			const int32_t id = readInt(in);
			const std::string& name = readString(in);
			const std::string& type = readString(in);
			const std::string& parameters = readString(in);
			const std::string& conditionType = readString(in);
			const std::string& conditionParameters = readString(in);
			const AIStateNodeStatic staticData(id, name, type, parameters, conditionType, conditionParameters);
			_nodeStaticData.push_back(staticData);
		}
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		addInt(out, _chrId);
		const std::size_t size = _nodeStaticDataPtr->size();
		addInt(out, static_cast<int>(size));
		for (std::size_t i = 0; i < size; ++i) {
			addInt(out, (*_nodeStaticDataPtr)[i].getId());
			addString(out, (*_nodeStaticDataPtr)[i].getName());
			addString(out, (*_nodeStaticDataPtr)[i].getType());
			addString(out, (*_nodeStaticDataPtr)[i].getParameters());
			addString(out, (*_nodeStaticDataPtr)[i].getConditionType());
			addString(out, (*_nodeStaticDataPtr)[i].getConditionParameters());
		}
	}

	inline const std::vector<AIStateNodeStatic>& getStaticNodeData() const {
		if (_nodeStaticDataPtr)
			return *_nodeStaticDataPtr;
		return _nodeStaticData;
	}
};

}
