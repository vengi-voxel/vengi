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
 * Deletes a node
 */
class AIDeleteNodeMessage: public IProtocolMessage {
private:
	int32_t _nodeId;
	CharacterId _characterId;

public:
	AIDeleteNodeMessage(int32_t nodeId, CharacterId characterId) :
			IProtocolMessage(PROTO_DELETENODE), _nodeId(nodeId), _characterId(characterId) {
	}

	explicit AIDeleteNodeMessage(streamContainer& in) :
			IProtocolMessage(PROTO_DELETENODE) {
		_nodeId = readInt(in);
		_characterId = readInt(in);
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		addInt(out, _nodeId);
		addInt(out, _characterId);
	}

	inline uint32_t getNodeId() const {
		return _nodeId;
	}

	inline CharacterId getCharacterId() const {
		return _characterId;
	}
};

}
