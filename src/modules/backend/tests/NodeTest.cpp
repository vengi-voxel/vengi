/**
 * @file
 */

#include "TestShared.h"
#include "backend/entity/ai/tree/Fail.h"
#include "backend/entity/ai/tree/Limit.h"
#include "backend/entity/ai/tree/Invert.h"
#include "backend/entity/ai/tree/Idle.h"
#include "backend/entity/ai/tree/Parallel.h"
#include "backend/entity/ai/tree/PrioritySelector.h"
#include "backend/entity/ai/tree/ProbabilitySelector.h"
#include "backend/entity/ai/tree/RandomSelector.h"
#include "backend/entity/ai/tree/Sequence.h"
#include "backend/entity/ai/tree/Steer.h"
#include "backend/entity/ai/tree/Succeed.h"
#include "backend/entity/ai/condition/And.h"
#include "backend/entity/ai/condition/False.h"
#include "backend/entity/ai/condition/HasEnemies.h"
#include "backend/entity/ai/condition/Not.h"
#include "backend/entity/ai/condition/Filter.h"
#include "backend/entity/ai/condition/Or.h"
#include "backend/entity/ai/condition/True.h"
#include "backend/entity/ai/condition/IsInGroup.h"
#include "backend/entity/ai/condition/IsGroupLeader.h"
#include "backend/entity/ai/condition/IsCloseToGroup.h"
#include <memory>

namespace backend {

class NodeTest: public TestSuite {
};

TEST_F(NodeTest, testSequence) {
	backend::Sequence::Factory f;
	backend::TreeNodeFactoryContext ctx("testsequence", "", backend::True::get());
	TreeNodePtr node = f.create(&ctx);

	backend::Idle::Factory idleFac;
	backend::TreeNodeFactoryContext idleCtx1("testidle", "2", backend::True::get());
	TreeNodePtr idle1 = idleFac.create(&idleCtx1);
	backend::TreeNodeFactoryContext idleCtx2("testidle2", "2", backend::True::get());
	TreeNodePtr idle2 = idleFac.create(&idleCtx2);

	node->addChild(idle1);
	node->addChild(idle2);

	AIPtr ai = std::make_shared<AI>(node);
	ICharacterPtr chr = core::make_shared<ICharacter>(1);
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
	backend::Idle::Factory f;
	backend::TreeNodeFactoryContext ctx("testidle", "1000", backend::True::get());
	TreeNodePtr node = f.create(&ctx);
	AIPtr entity = std::make_shared<AI>(node);
	ICharacterPtr chr = core::make_shared<ICharacter>(1);
	entity->setCharacter(chr);
	ASSERT_EQ(ai::RUNNING, node->execute(entity, 1));
	ASSERT_EQ(ai::FINISHED, node->execute(entity, 1000));
}

TEST_F(NodeTest, testParallel) {
	backend::Parallel::Factory f;
	backend::TreeNodeFactoryContext ctx("testparallel", "", backend::True::get());
	TreeNodePtr node = f.create(&ctx);

	backend::Idle::Factory idleFac;
	backend::TreeNodeFactoryContext idleCtx1("testidle", "2", backend::True::get());
	TreeNodePtr idle1 = idleFac.create(&idleCtx1);
	backend::TreeNodeFactoryContext idleCtx2("testidle2", "2", backend::True::get());
	TreeNodePtr idle2 = idleFac.create(&idleCtx2);

	node->addChild(idle1);
	node->addChild(idle2);

	AIPtr e = std::make_shared<AI>(node);
	ICharacterPtr chr = core::make_shared<ICharacter>(1);
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
	backend::PrioritySelector::Factory f;
	backend::TreeNodeFactoryContext ctx("testpriorityselector", "", backend::True::get());
	TreeNodePtr node = f.create(&ctx);

	backend::Idle::Factory idleFac;
	backend::TreeNodeFactoryContext idleCtx1("testidle", "2", backend::True::get());
	TreeNodePtr idle1 = idleFac.create(&idleCtx1);
	backend::TreeNodeFactoryContext idleCtx2("testidle2", "2", backend::True::get());
	TreeNodePtr idle2 = idleFac.create(&idleCtx2);

	node->addChild(idle1);
	node->addChild(idle2);

	AIPtr e = std::make_shared<AI>(node);
	ICharacterPtr chr = core::make_shared<ICharacter>(1);
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
	backend::PrioritySelector::Factory f;
	backend::TreeNodeFactoryContext ctx("testpriorityselector", "", backend::True::get());
	TreeNodePtr node = f.create(&ctx);

	backend::Idle::Factory idleFac;
	backend::TreeNodeFactoryContext idleCtx1("testidle", "2", backend::False::get());
	TreeNodePtr idle1 = idleFac.create(&idleCtx1);
	backend::TreeNodeFactoryContext idleCtx2("testidle2", "2", backend::True::get());
	TreeNodePtr idle2 = idleFac.create(&idleCtx2);

	node->addChild(idle1);
	node->addChild(idle2);

	AIPtr e = std::make_shared<AI>(node);
	ICharacterPtr chr = core::make_shared<ICharacter>(1);
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

}
