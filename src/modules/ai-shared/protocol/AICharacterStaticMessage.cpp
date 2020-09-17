/**
 * @file
 */

#include "AICharacterStaticMessage.h"

namespace ai {

AICharacterStaticMessage::AICharacterStaticMessage(const CharacterId& id, const core::DynamicArray<AIStateNodeStatic>& nodeStaticData) :
		IProtocolMessage(PROTO_CHARACTER_STATIC), _chrId(id), _nodeStaticDataPtr(&nodeStaticData) {
}

AICharacterStaticMessage::AICharacterStaticMessage(streamContainer& in) :
		IProtocolMessage(PROTO_CHARACTER_STATIC), _nodeStaticDataPtr(nullptr) {
	_chrId = readInt(in);
	const size_t size = readInt(in);
	_nodeStaticData.reserve(size);
	for (size_t i = 0u; i < size; ++i) {
		const int32_t id = readInt(in);
		const core::String& name = readString(in);
		const core::String& type = readString(in);
		const core::String& parameters = readString(in);
		const core::String& conditionType = readString(in);
		const core::String& conditionParameters = readString(in);
		const AIStateNodeStatic staticData(id, name, type, parameters, conditionType, conditionParameters);
		_nodeStaticData.push_back(staticData);
	}
}

void AICharacterStaticMessage::serialize(streamContainer& out) const {
	addByte(out, _id);
	addInt(out, _chrId);
	const size_t size = _nodeStaticDataPtr->size();
	addInt(out, static_cast<int>(size));
	for (size_t i = 0u; i < size; ++i) {
		addInt(out, (*_nodeStaticDataPtr)[i].getId());
		addString(out, (*_nodeStaticDataPtr)[i].getName());
		addString(out, (*_nodeStaticDataPtr)[i].getType());
		addString(out, (*_nodeStaticDataPtr)[i].getParameters());
		addString(out, (*_nodeStaticDataPtr)[i].getConditionType());
		addString(out, (*_nodeStaticDataPtr)[i].getConditionParameters());
	}
}

}
