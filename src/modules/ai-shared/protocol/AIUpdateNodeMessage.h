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
 * Updates a node to some other type, name or condition
 */
class AIUpdateNodeMessage: public IProtocolMessage {
private:
	int32_t _nodeId;
	CharacterId _characterId;
	core::String _name;
	core::String _type;
	core::String _condition;

public:
	AIUpdateNodeMessage(int32_t nodeId, CharacterId characterId, const core::String& name, const core::String& type,
			const core::String& condition) :
			IProtocolMessage(PROTO_UPDATENODE), _nodeId(nodeId), _characterId(characterId), _name(name), _type(type), _condition(condition) {
	}

	explicit AIUpdateNodeMessage(streamContainer& in) :
			IProtocolMessage(PROTO_UPDATENODE) {
		_nodeId = readInt(in);
		_characterId = readInt(in);
		_name = readString(in);
		_type = readString(in);
		_condition = readString(in);
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		addInt(out, _nodeId);
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

	inline uint32_t getNodeId() const {
		return _nodeId;
	}

	inline CharacterId getCharacterId() const {
		return _characterId;
	}
};

}
