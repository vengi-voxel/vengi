/**
 * @file
 */
#pragma once

#include "IProtocolMessage.h"
#include "AIStubTypes.h"

namespace ai {

/**
 * @brief Message for the remote debugging interface
 *
 * Adds a new node to some parent node
 */
class AIAddNodeMessage: public IProtocolMessage {
private:
	int32_t _parentNodeId;
	CharacterId _characterId;
	core::String _name;
	core::String _type;
	core::String _condition;

public:
	AIAddNodeMessage(int32_t parentNodeId, CharacterId characterId, const core::String& name, const core::String& type, const core::String& condition) :
			IProtocolMessage(PROTO_ADDNODE), _parentNodeId(parentNodeId), _characterId(characterId), _name(name), _type(type), _condition(condition) {
	}

	explicit AIAddNodeMessage(streamContainer& in) :
			IProtocolMessage(PROTO_ADDNODE) {
		_parentNodeId = readInt(in);
		_characterId = readInt(in);
		_name = readString(in);
		_type = readString(in);
		_condition = readString(in);
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		addInt(out, _parentNodeId);
		addInt(out, _characterId);
		addString(out, _name);
		addString(out, _type);
		addString(out, _condition);
	}

	inline const core::String& getName() const {
		return _name;
	}

	inline const core::String& getType() const {
		return _type;
	}

	inline const core::String& getCondition() const {
		return _condition;
	}

	inline uint32_t getParentNodeId() const {
		return _parentNodeId;
	}

	inline CharacterId getCharacterId() const {
		return _characterId;
	}
};

}
