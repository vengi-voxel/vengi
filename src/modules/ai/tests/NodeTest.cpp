/**
 * @file
 */

#include "TestShared.h"
#include "tree/Fail.h"
#include "tree/Limit.h"
#include "tree/Invert.h"
#include "tree/Idle.h"
#include "tree/Parallel.h"
#include "tree/PrioritySelector.h"
#include "tree/ProbabilitySelector.h"
#include "tree/RandomSelector.h"
#include "tree/Sequence.h"
#include "tree/Steer.h"
#include "tree/Succeed.h"
#include "conditions/And.h"
#include "conditions/False.h"
#include "conditions/HasEnemies.h"
#include "conditions/Not.h"
#include "conditions/Filter.h"
#include "conditions/Or.h"
#include "conditions/True.h"
#include "conditions/IsInGroup.h"
#include "conditions/IsGroupLeader.h"
#include "conditions/IsCloseToGroup.h"

class NodeTest: public TestSuite {
};

TEST_F(NodeTest, testSequence) {
	ai::Sequence::Factory f;
	ai::TreeNodeFactoryContext ctx("testsequence", "", ai::True::get());
	ai::TreeNodePtr node = f.create(&ctx);

	ai::Idle::Factory idleFac;
	ai::TreeNodeFactoryContext idleCtx1("testidle", "2", ai::True::get());
	ai::TreeNodePtr idle1 = idleFac.create(&idleCtx1);
	ai::TreeNodeFactoryContext idleCtx2("testidle2", "2", ai::True::get());
	ai::TreeNodePtr idle2 = idleFac.create(&idleCtx2);

	node->addChild(idle1);
	node->addChild(idle2);

	ai::AIPtr ai(new ai::AI(node));
	ai::ICharacterPtr chr(new ai::ICharacter(1));
	ai->setCharacter(chr);
	ai->update(1, true);
	ai->getBehaviour()->execute(ai, 1);
	ASSERT_EQ(ai::RUNNING, idle1->getLastStatus(ai));
	ASSERT_EQ(ai::UNKNOWN, idle2->getLastStatus(ai));
	ai->update(1, true);
	ai->getBehaviour()->execute(ai, 1);
	ASSERT_EQ(ai::RUNNING, idle1->getLastStatus(ai));
	ASSERT_EQ(ai::UNKNOWN, idle2->getLastStatus(ai));
	ai->update(1, true);
	ai->getBehaviour()->execute(ai, 1);
	ASSERT_EQ(ai::FINISHED, idle1->getLastStatus(ai));
	ASSERT_EQ(ai::RUNNING, idle2->getLastStatus(ai));
	ai->update(1, true);
	ai->getBehaviour()->execute(ai, 1);
	ASSERT_EQ(ai::FINISHED, idle1->getLastStatus(ai));
	ASSERT_EQ(ai::RUNNING, idle2->getLastStatus(ai));
	ai->update(1, true);
	ai->getBehaviour()->execute(ai, 1);
	ASSERT_EQ(ai::FINISHED, idle1->getLastStatus(ai));
	ASSERT_EQ(ai::FINISHED, idle2->getLastStatus(ai));
	ai->update(1, true);
	ai->getBehaviour()->execute(ai, 1);
	ASSERT_EQ(ai::RUNNING, idle1->getLastStatus(ai));
	ASSERT_EQ(ai::FINISHED, idle2->getLastStatus(ai));
}

TEST_F(NodeTest, testIdle) {
	ai::Idle::Factory f;
	ai::TreeNodeFactoryContext ctx("testidle", "1000", ai::True::get());
	ai::TreeNodePtr node = f.create(&ctx);
	ai::AIPtr entity(new ai::AI(node));
	ai::ICharacterPtr chr(new ai::ICharacter(1));
	entity->setCharacter(chr);
	ASSERT_EQ(ai::RUNNING, node->execute(entity, 1));
	ASSERT_EQ(ai::FINISHED, node->execute(entity, 1000));
}

TEST_F(NodeTest, testParallel) {
	ai::Parallel::Factory f;
	ai::TreeNodeFactoryContext ctx("testparallel", "", ai::True::get());
	ai::TreeNodePtr node = f.create(&ctx);

	ai::Idle::Factory idleFac;
	ai::TreeNodeFactoryContext idleCtx1("testidle", "2", ai::True::get());
	ai::TreeNodePtr idle1 = idleFac.create(&idleCtx1);
	ai::TreeNodeFactoryContext idleCtx2("testidle2", "2", ai::True::get());
	ai::TreeNodePtr idle2 = idleFac.create(&idleCtx2);

	node->addChild(idle1);
	node->addChild(idle2);

	ai::AIPtr e(new ai::AI(node));
	ai::ICharacterPtr chr(new ai::ICharacter(1));
	e->setCharacter(chr);
	e->update(1, true);
	e->getBehaviour()->execute(e, 1);
	ASSERT_EQ(ai::RUNNING, idle1->getLastStatus(e));
	ASSERT_EQ(ai::RUNNING, idle2->getLastStatus(e));
	e->update(1, true);
	e->getBehaviour()->execute(e, 1);
	ASSERT_EQ(ai::RUNNING, idle1->getLastStatus(e));
	ASSERT_EQ(ai::RUNNING, idle2->getLastStatus(e));
	e->update(1, true);
	e->getBehaviour()->execute(e, 1);
	ASSERT_EQ(ai::FINISHED, idle1->getLastStatus(e));
	ASSERT_EQ(ai::FINISHED, idle2->getLastStatus(e));
}

TEST_F(NodeTest, testPrioritySelector) {
	ai::PrioritySelector::Factory f;
	ai::TreeNodeFactoryContext ctx("testpriorityselector", "", ai::True::get());
	ai::TreeNodePtr node = f.create(&ctx);

	ai::Idle::Factory idleFac;
	ai::TreeNodeFactoryContext idleCtx1("testidle", "2", ai::True::get());
	ai::TreeNodePtr idle1 = idleFac.create(&idleCtx1);
	ai::TreeNodeFactoryContext idleCtx2("testidle2", "2", ai::True::get());
	ai::TreeNodePtr idle2 = idleFac.create(&idleCtx2);

	node->addChild(idle1);
	node->addChild(idle2);

	ai::AIPtr e(new ai::AI(node));
	ai::ICharacterPtr chr(new ai::ICharacter(1));
	e->setCharacter(chr);
	e->update(1, true);
	e->getBehaviour()->execute(e, 1);
	ASSERT_EQ(ai::RUNNING, idle1->getLastStatus(e));
	ASSERT_EQ(ai::UNKNOWN, idle2->getLastStatus(e));
	e->update(1, true);
	e->getBehaviour()->execute(e, 1);
	ASSERT_EQ(ai::RUNNING, idle1->getLastStatus(e));
	ASSERT_EQ(ai::UNKNOWN, idle2->getLastStatus(e));
	e->update(1, true);
	e->getBehaviour()->execute(e, 1);
	ASSERT_EQ(ai::FINISHED, idle1->getLastStatus(e));
	ASSERT_EQ(ai::UNKNOWN, idle2->getLastStatus(e));
}

TEST_F(NodeTest, testPrioritySelectorWithCondition) {
	ai::PrioritySelector::Factory f;
	ai::TreeNodeFactoryContext ctx("testpriorityselector", "", ai::True::get());
	ai::TreeNodePtr node = f.create(&ctx);

	ai::Idle::Factory idleFac;
	ai::TreeNodeFactoryContext idleCtx1("testidle", "2", ai::False::get());
	ai::TreeNodePtr idle1 = idleFac.create(&idleCtx1);
	ai::TreeNodeFactoryContext idleCtx2("testidle2", "2", ai::True::get());
	ai::TreeNodePtr idle2 = idleFac.create(&idleCtx2);

	node->addChild(idle1);
	node->addChild(idle2);

	ai::AIPtr e(new ai::AI(node));
	ai::ICharacterPtr chr(new ai::ICharacter(1));
	e->setCharacter(chr);
	e->update(1, true);
	e->getBehaviour()->execute(e, 1);
	ASSERT_EQ(ai::CANNOTEXECUTE, idle1->getLastStatus(e));
	ASSERT_EQ(ai::RUNNING, idle2->getLastStatus(e));
	e->update(1, true);
	e->getBehaviour()->execute(e, 1);
	ASSERT_EQ(ai::CANNOTEXECUTE, idle1->getLastStatus(e));
	ASSERT_EQ(ai::RUNNING, idle2->getLastStatus(e));
	e->update(1, true);
	e->getBehaviour()->execute(e, 1);
	ASSERT_EQ(ai::CANNOTEXECUTE, idle1->getLastStatus(e));
	ASSERT_EQ(ai::FINISHED, idle2->getLastStatus(e));
}
