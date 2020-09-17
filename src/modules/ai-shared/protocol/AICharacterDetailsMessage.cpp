/**
 * @file
 */

#include "AICharacterDetailsMessage.h"
#include "core/Enum.h"
#include "core/collection/DynamicArray.h"

namespace ai {

AIStateNode AICharacterDetailsMessage::readNode (streamContainer& in) {
	const int32_t nodeId = readInt(in);
	const core::String& condition = readString(in);
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

void AICharacterDetailsMessage::writeNode (streamContainer& out, const AIStateNode& node) const {
	addInt(out, node.getNodeId());
	addString(out, node.getCondition());
	addLong(out, node.getLastRun());
	addByte(out, core::enumVal(node.getStatus()));
	addBool(out, node.isRunning());
	const core::DynamicArray<AIStateNode>& children = node.getChildren();
	addShort(out, static_cast<int16_t>(children.size()));
	for (core::DynamicArray<AIStateNode>::iterator i = children.begin(); i != children.end(); ++i) {
		writeNode(out, *i);
	}
}

void AICharacterDetailsMessage::writeAggro(streamContainer& out, const AIStateAggro& aggro) const {
	const core::DynamicArray<AIStateAggroEntry>& a = aggro.getAggro();
	addShort(out, static_cast<int16_t>(a.size()));
	for (core::DynamicArray<AIStateAggroEntry>::iterator i = a.begin(); i != a.end(); ++i) {
		addInt(out, i->id);
		addFloat(out, i->aggro);
	}
}

void AICharacterDetailsMessage::readAggro(streamContainer& in, AIStateAggro& aggro) const {
	const int size = readShort(in);
	for (int i = 0; i < size; ++i) {
		const CharacterId chrId = readInt(in);
		const float aggroVal = readFloat(in);
		aggro.addAggro(AIStateAggroEntry(chrId, aggroVal));
	}
}

AICharacterDetailsMessage::AICharacterDetailsMessage(const CharacterId& id, const AIStateAggro& aggro, const AIStateNode& root) :
		IProtocolMessage(PROTO_CHARACTER_DETAILS), _chrId(id), _aggroPtr(&aggro), _rootPtr(&root) {
}

AICharacterDetailsMessage::AICharacterDetailsMessage(streamContainer& in) :
		IProtocolMessage(PROTO_CHARACTER_DETAILS), _aggroPtr(nullptr), _rootPtr(nullptr) {
	_chrId = readInt(in);
	readAggro(in, _aggro);
	_root = readNode(in);
}

void AICharacterDetailsMessage::serialize(streamContainer& out) const {
	addByte(out, _id);
	addInt(out, _chrId);
	writeAggro(out, *_aggroPtr);
	writeNode(out, *_rootPtr);
}

}
