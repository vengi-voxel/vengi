/**
 * @file
 */

#include "NpcTest.h"
#include "backend/entity/ai/filter/SelectIncreasePartner.h"
#include "backend/entity/ai/filter/SelectVisible.h"
#include "backend/entity/ai/filter/SelectEntitiesOfTypes.h"
#include "backend/entity/ai/condition/IsSelectionAlive.h"
#include "backend/entity/ai/condition/IsOnCooldown.h"
#include "backend/entity/ai/condition/IsCloseToSelection.h"
#include "backend/entity/ai/action/AttackOnSelection.h"
#include "backend/entity/ai/action/Die.h"
#include "backend/entity/ai/action/GoHome.h"
#include "backend/entity/ai/action/SetPointOfInterest.h"
#include "backend/entity/ai/action/Spawn.h"
#include "backend/entity/ai/action/TriggerCooldown.h"
#include "backend/entity/ai/action/TriggerCooldownOnSelection.h"

namespace backend {

class AITest: public NpcTest {
private:
	using Super = NpcTest;
};

TEST_F(AITest, testFilterSelectIncreasePartner) {
	const NpcPtr& npc = create();
	const NpcPtr& partner = setVisible(npc);
	EXPECT_EQ(partner->id(), npc->ai()->getFilteredEntities().front());
}

TEST_F(AITest, testFilterSelectVisible) {
	const NpcPtr& npc = create();
	const NpcPtr& npc2 = create();
	const NpcPtr& npc3 = create();
	const NpcPtr& npcNotVisible = create();
	npc->updateVisible({npc2, npc3});

	const ai::FilterFactoryContext ctx("");
	const ai::FilterPtr& filter = SelectVisible::getFactory().create(&ctx);
	filter->filter(npc->ai());

	const ai::FilteredEntities& fe = npc->ai()->getFilteredEntities();
	EXPECT_EQ(2u, fe.size())
		<< "Expected to have two of the npcs visible, but " << fe.size() << " are";
	EXPECT_TRUE(std::find(fe.begin(), fe.end(), npcNotVisible->id()) == fe.end())
		<< "This npc should not be part of the visible set";
}

TEST_F(AITest, testFilterSelectEntitiesOfTypes) {
	const NpcPtr& npc = create(network::EntityType::ANIMAL_RABBIT);
	const NpcPtr& typeOne1 = create(network::EntityType::ANIMAL_RABBIT);
	const NpcPtr& typeOne2 = create(network::EntityType::ANIMAL_RABBIT);
	const NpcPtr& typeTwo1 = create(network::EntityType::ANIMAL_WOLF);
	const NpcPtr& typeTwo2 = create(network::EntityType::ANIMAL_WOLF);
	npc->updateVisible({typeOne1, typeTwo1, typeOne2, typeTwo2});

	const ai::FilterFactoryContext ctx(network::EnumNameEntityType(npc->entityType()));
	const ai::FilterPtr& filter = SelectEntitiesOfTypes::getFactory().create(&ctx);
	filter->filter(npc->ai());

	const ai::FilteredEntities& fe = npc->ai()->getFilteredEntities();
	EXPECT_EQ(2u, fe.size())
		<< "Expected to have two of the npcs visible, but " << fe.size() << " are";
}

TEST_F(AITest, testConditionIsSelectionAlive) {
	const NpcPtr& npc = create();
	setVisible(npc);
	const ai::ConditionFactoryContext ctx("");
	const ai::ConditionPtr& condition = IsSelectionAlive::getFactory().create(&ctx);
	EXPECT_TRUE(condition->evaluate(npc->ai())) << "NPC should be alive";
}

TEST_F(AITest, testConditionIsClosetoSelection) {
	const NpcPtr& npc = create();
	setVisible(npc);
	const ai::ConditionFactoryContext ctx("");
	const ai::ConditionPtr& condition = IsCloseToSelection::getFactory().create(&ctx);
	EXPECT_TRUE(condition->evaluate(npc->ai())) << "NPCs should be close to each other";
}

TEST_F(AITest, testConditionIsOnCooldown) {
	const NpcPtr& npc = create();
	const ai::ConditionFactoryContext ctx(network::EnumNameCooldownType(cooldown::Type::INCREASE));
	const ai::ConditionPtr& condition = IsOnCooldown::getFactory().create(&ctx);
	EXPECT_EQ(cooldown::CooldownTriggerState::SUCCESS, npc->cooldownMgr().triggerCooldown(cooldown::Type::INCREASE));
	EXPECT_TRUE(condition->evaluate(npc->ai())) << "NPC should have the cooldown triggered";
}

TEST_F(AITest, testActionTriggerCooldown) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", network::EnumNameCooldownType(cooldown::Type::INCREASE), ai::True::get());
	const ai::TreeNodePtr& action = TriggerCooldown::getFactory().create(&ctx);
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	EXPECT_TRUE(npc->cooldownMgr().isCooldown(cooldown::Type::INCREASE));
}

TEST_F(AITest, testActionTriggerCooldownOnSelection) {
	const NpcPtr& npc = create();
	setVisible(npc);
	const ai::TreeNodeFactoryContext ctx("foo", network::EnumNameCooldownType(cooldown::Type::INCREASE), ai::True::get());
	const ai::TreeNodePtr& action = TriggerCooldown::getFactory().create(&ctx);
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
}

TEST_F(AITest, testActionSpawn) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", "", ai::True::get());
	const ai::TreeNodePtr& action = Spawn::getFactory().create(&ctx);
	const int before = map->npcCount();
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	const int after = map->npcCount();
	EXPECT_EQ(before + 1, after) << "NPC wasn't spawned as expected";
}

TEST_F(AITest, testActionSetPointOfInterest) {
	const NpcPtr& npc = create();
	const size_t before = map->poiProvider()->count();
	const ai::TreeNodeFactoryContext ctx("foo", "", ai::True::get());
	const ai::TreeNodePtr& action = SetPointOfInterest::getFactory().create(&ctx);
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	EXPECT_GT(map->poiProvider()->count(), before);
}

TEST_F(AITest, testActionGoHome) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", "", ai::True::get());
	const ai::TreeNodePtr& action = GoHome::getFactory().create(&ctx);
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
}

TEST_F(AITest, testActionDie) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", "", ai::True::get());
	const ai::TreeNodePtr& action = Die::getFactory().create(&ctx);
	EXPECT_FALSE(npc->dead()) << "NPC should be alive";
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	EXPECT_TRUE(npc->dead()) << "NPC should be dead";
}

TEST_F(AITest, testActionAttackOnSelection) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", "", ai::True::get());
	const ai::TreeNodePtr& action = AttackOnSelection::getFactory().create(&ctx);
	EXPECT_EQ(ai::TreeNodeStatus::FAILED, action->execute(npc->ai(), 0L));
	setVisible(npc);
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
}

}
