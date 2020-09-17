/**
 * @file
 */

#include "TestShared.h"
#include "ai-shared/protocol/ProtocolMessageFactory.h"
#include "ai-shared/protocol/AIPauseMessage.h"
#include "ai-shared/protocol/AISelectMessage.h"
#include "ai-shared/protocol/AIChangeMessage.h"
#include "ai-shared/protocol/AINamesMessage.h"
#include "ai-shared/protocol/AICharacterDetailsMessage.h"
#include "ai-shared/protocol/AIStateMessage.h"
#include "core/collection/DynamicArray.h"

class MessageTest: public TestSuite {
protected:
	template <typename T>
	T* serializeDeserialize(const T& msg) const {
		ai::streamContainer stream;
		// fake the size that is used in the network stream
		ai::IProtocolMessage::addInt(stream, 0);
		msg.serialize(stream);
		ai::ProtocolMessageFactory& f = ai::ProtocolMessageFactory::get();
		ai::IProtocolMessage *deserialized = f.create(stream);
		return static_cast<T*>(deserialized);
	}
};

TEST_F(MessageTest, testAICharacterDetailsMessage) {
	const ai::CharacterId id = 1;
	ai::AIStateAggro aggro;
	const ai::AIStateAggroEntry aggroEntry(2, 1.0f);
	aggro.addAggro(aggroEntry);
	const ai::AIStateNode root(1, "condition", 1L, ai::TreeNodeStatus::RUNNING, true);
	const ai::AICharacterDetailsMessage m(id, aggro, root);
	ASSERT_EQ(id, m.getCharacterId());
	ASSERT_EQ(1u, m.getAggro().getAggro().size());
	ASSERT_EQ(2, m.getAggro().getAggro()[0].id);
	ASSERT_FLOAT_EQ(1.0f, m.getAggro().getAggro()[0].aggro);
	ASSERT_EQ(1L, m.getNode().getLastRun());
	ASSERT_EQ("condition", m.getNode().getCondition());
	ASSERT_EQ(1, m.getNode().getNodeId());
	ASSERT_EQ(ai::TreeNodeStatus::RUNNING, m.getNode().getStatus());
	ASSERT_TRUE(m.getNode().isRunning());

	ai::AICharacterDetailsMessage* d(serializeDeserialize(m));
	ASSERT_EQ(m.getId(), d->getId());
	ASSERT_EQ(id, d->getCharacterId());
	ASSERT_EQ(2, d->getAggro().getAggro()[0].id);
	ASSERT_FLOAT_EQ(1.0f, d->getAggro().getAggro()[0].aggro);
	ASSERT_EQ(1L, d->getNode().getLastRun());
	ASSERT_EQ("condition", d->getNode().getCondition());
	ASSERT_EQ(1, d->getNode().getNodeId());
	ASSERT_EQ(ai::TreeNodeStatus::RUNNING, d->getNode().getStatus());
	ASSERT_TRUE(d->getNode().isRunning());
}

TEST_F(MessageTest, testAIPauseMessage) {
	{
		ai::AIPauseMessage m(true);
		ai::AIPauseMessage* d = serializeDeserialize(m);
		ASSERT_EQ(m.getId(), d->getId());
		ASSERT_TRUE(d->isPause());
	}
	{
		ai::AIPauseMessage m(false);
		ai::AIPauseMessage* d = serializeDeserialize(m);
		ASSERT_EQ(m.getId(), d->getId());
		ASSERT_FALSE(d->isPause());
	}
}

TEST_F(MessageTest, testAINamesMessage) {
	core::DynamicArray<core::String> names;
	names.push_back("Test");
	ai::AINamesMessage m(names);
	ai::AINamesMessage* d = serializeDeserialize(m);
	ASSERT_EQ(m.getId(), d->getId());
	ASSERT_EQ("Test", d->getNames()[0]);
}

TEST_F(MessageTest, testAIChangeMessage) {
	ai::AIChangeMessage m("Test");
	ai::AIChangeMessage* d = serializeDeserialize(m);
	ASSERT_EQ(m.getId(), d->getId());
	ASSERT_EQ("Test", d->getName());
}

TEST_F(MessageTest, testAISelectMessage) {
	ai::AISelectMessage m(1);
	ai::AISelectMessage* d = serializeDeserialize(m);
	ASSERT_EQ(m.getId(), d->getId());
	ASSERT_EQ(1, d->getCharacterId());
}

TEST_F(MessageTest, testAIStateMessage) {
	ai::CharacterAttributes attributes;
	attributes.put("Name", "Test");
	attributes.put("SomethingElse", "SomeValue");

	ai::AIStateMessage m;
	ai::AIStateWorld state(1, backend::ZERO, 1.0f, attributes);
	ASSERT_EQ("Test", state.getAttributes().find("Name")->second);
	ASSERT_EQ("SomeValue", state.getAttributes().find("SomethingElse")->second);
	m.addState(state);

	ai::AIStateMessage* d = serializeDeserialize(m);
	ASSERT_EQ(m.getId(), d->getId());
	ASSERT_EQ(1u, d->getStates().size());
	ASSERT_EQ("Test", d->getStates()[0].getAttributes().find("Name")->second);
	ASSERT_EQ("SomeValue", d->getStates()[0].getAttributes().find("SomethingElse")->second);
	ASSERT_EQ(1, d->getStates()[0].getId());
	ASSERT_FLOAT_EQ(1.0f, d->getStates()[0].getOrientation());
}

TEST_F(MessageTest, testIProtocolMessageStep) {
	ai::IProtocolMessage m(ai::PROTO_STEP);
	ai::IProtocolMessage* d = serializeDeserialize(m);
	ASSERT_EQ(m.getId(), d->getId());
}

TEST_F(MessageTest, testIProtocolMessageReset) {
	ai::IProtocolMessage m(ai::PROTO_RESET);
	ai::IProtocolMessage* d = serializeDeserialize(m);
	ASSERT_EQ(m.getId(), d->getId());
}
