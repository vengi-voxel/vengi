#pragma once

#include "IProtocolMessage.h"
#include "AIStubTypes.h"
#include <vector>

namespace ai {

/**
 * @brief Message for the remote debugging interface
 *
 * If someone selected a character this message gets broadcasted.
 */
class AICharacterDetailsMessage: public IProtocolMessage {
private:
	CharacterId _chrId;
	const AIStateAggro* _aggroPtr;
	const AIStateNode* _rootPtr;
	AIStateAggro _aggro;
	AIStateNode _root;

	AIStateNode readNode (streamContainer& in) {
		const int32_t nodeId = readInt(in);
		const std::string& condition = readString(in);
		const int64_t lastRun = readLong(in);
		const TreeNodeStatus status = static_cast<TreeNodeStatus>(readByte(in));
		const bool running = readBool(in);
		const int16_t childrenCount = readShort(in);
		AIStateNode node(nodeId, condition, lastRun, status, running);
		for (uint8_t i = 0; i < childrenCount; ++i) {
			const AIStateNode& child = readNode(in);
			node.addChildren(child);
		}
		return node;
	}

	void writeNode (streamContainer& out, const AIStateNode& node) const {
		addInt(out, node.getNodeId());
		addString(out, node.getCondition());
		addLong(out, node.getLastRun());
		addByte(out, node.getStatus());
		addBool(out, node.isRunning());
		const std::vector<AIStateNode>& children = node.getChildren();
		addShort(out, static_cast<int16_t>(children.size()));
		for (std::vector<AIStateNode>::const_iterator i = children.begin(); i != children.end(); ++i) {
			writeNode(out, *i);
		}
	}

	void writeAggro(streamContainer& out, const AIStateAggro& aggro) const {
		const std::vector<AIStateAggroEntry>& a = aggro.getAggro();
		addShort(out, static_cast<int16_t>(a.size()));
		for (std::vector<AIStateAggroEntry>::const_iterator i = a.begin(); i != a.end(); ++i) {
			addInt(out, i->id);
			addFloat(out, i->aggro);
		}
	}

	void readAggro(streamContainer& in, AIStateAggro& aggro) const {
		const int size = readShort(in);
		for (int i = 0; i < size; ++i) {
			const CharacterId chrId = readInt(in);
			const float aggroVal = readFloat(in);
			aggro.addAggro(AIStateAggroEntry(chrId, aggroVal));
		}
	}

public:
	/**
	 * Make sure that none of the given references is destroyed, for performance reasons we are only storing the
	 * pointers to those instances in this class. So they need to stay valid until they are serialized.
	 */
	AICharacterDetailsMessage(const CharacterId& id, const AIStateAggro& aggro, const AIStateNode& root) :
			IProtocolMessage(PROTO_CHARACTER_DETAILS), _chrId(id), _aggroPtr(&aggro), _rootPtr(&root) {
	}

	explicit AICharacterDetailsMessage(streamContainer& in) :
			IProtocolMessage(PROTO_CHARACTER_DETAILS), _aggroPtr(nullptr), _rootPtr(nullptr) {
		_chrId = readInt(in);
		readAggro(in, _aggro);
		_root = readNode(in);
	}

	void serialize(streamContainer& out) const override {
		addByte(out, _id);
		addInt(out, _chrId);
		writeAggro(out, *_aggroPtr);
		writeNode(out, *_rootPtr);
	}

	inline const CharacterId& getCharacterId() const {
		return _chrId;
	}

	inline const AIStateAggro& getAggro() const {
		if (_aggroPtr)
			return *_aggroPtr;
		return _aggro;
	}

	inline const AIStateNode& getNode() const {
		if (_rootPtr)
			return *_rootPtr;
		return _root;
	}
};

}
