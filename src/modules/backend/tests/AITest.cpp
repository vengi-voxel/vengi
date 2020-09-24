/**
 * @file
 */

#include "NpcTest.h"
#include "backend/entity/ai/filter/Complement.h"
#include "backend/entity/ai/filter/Difference.h"
#include "backend/entity/ai/filter/FilteredEntities.h"
#include "backend/entity/ai/filter/First.h"
#include "backend/entity/ai/filter/Intersection.h"
#include "backend/entity/ai/filter/Last.h"
#include "backend/entity/ai/filter/SelectIncreasePartner.h"
#include "backend/entity/ai/filter/SelectVisible.h"
#include "backend/entity/ai/filter/SelectEntitiesOfTypes.h"
#include "backend/entity/ai/condition/IsSelectionAlive.h"
#include "backend/entity/ai/condition/IsOnCooldown.h"
#include "backend/entity/ai/condition/IsCloseToSelection.h"
#include "backend/entity/ai/condition/True.h"
#include "backend/entity/ai/action/AttackOnSelection.h"
#include "backend/entity/ai/action/Die.h"
#include "backend/entity/ai/action/GoHome.h"
#include "backend/entity/ai/action/SetPointOfInterest.h"
#include "backend/entity/ai/action/Spawn.h"
#include "backend/entity/ai/action/TriggerCooldown.h"
#include "backend/entity/ai/action/TriggerCooldownOnSelection.h"
#include "backend/entity/ai/filter/Union.h"
#include "core/Tokenizer.h"

namespace backend {

class AITest: public NpcTest {
private:
	using Super = NpcTest;
};

class FakeFilter: public IFilter {
public:
	FILTER_ACTION_CLASS(FakeFilter)
	FILTER_ACTION_FACTORY(FakeFilter)

	void filter (const AIPtr& entity) override {
		core::Tokenizer tok(_parameters, ",");
		while (tok.hasNext()) {
			entity->addFilteredEntity(core::string::toInt(tok.next()));
		}
	}
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

	const backend::FilterFactoryContext ctx("");
	const backend::FilterPtr& filter = SelectVisible::getFactory().create(&ctx);
	filter->filter(npc->ai());

	const backend::FilteredEntities& fe = npc->ai()->getFilteredEntities();
	EXPECT_EQ(2u, fe.size())
		<< "Expected to have two of the npcs visible, but " << fe.size() << " are";
	EXPECT_TRUE(core::find(fe.begin(), fe.end(), npcNotVisible->id()) == fe.end())
		<< "This npc should not be part of the visible set";
}

TEST_F(AITest, testFilterSelectEntitiesOfTypes) {
	const NpcPtr& npc = create(network::EntityType::ANIMAL_RABBIT);
	const NpcPtr& typeOne1 = create(network::EntityType::ANIMAL_RABBIT);
	const NpcPtr& typeOne2 = create(network::EntityType::ANIMAL_RABBIT);
	const NpcPtr& typeTwo1 = create(network::EntityType::ANIMAL_WOLF);
	const NpcPtr& typeTwo2 = create(network::EntityType::ANIMAL_WOLF);
	npc->updateVisible({typeOne1, typeTwo1, typeOne2, typeTwo2});

	const backend::FilterFactoryContext ctx(network::EnumNameEntityType(npc->entityType()));
	const backend::FilterPtr& filter = SelectEntitiesOfTypes::getFactory().create(&ctx);
	filter->filter(npc->ai());

	const backend::FilteredEntities& fe = npc->ai()->getFilteredEntities();
	EXPECT_EQ(2u, fe.size())
		<< "Expected to have two of the npcs visible, but " << fe.size() << " are";
}

TEST_F(AITest, testConditionIsSelectionAlive) {
	const NpcPtr& npc = create();
	setVisible(npc);
	const backend::ConditionFactoryContext ctx("");
	const backend::ConditionPtr& condition = IsSelectionAlive::getFactory().create(&ctx);
	EXPECT_TRUE(condition->evaluate(npc->ai())) << "NPC should be alive";
}

TEST_F(AITest, testConditionIsClosetoSelection) {
	const NpcPtr& npc = create();
	setVisible(npc);
	const backend::ConditionFactoryContext ctx("");
	const backend::ConditionPtr& condition = IsCloseToSelection::getFactory().create(&ctx);
	EXPECT_TRUE(condition->evaluate(npc->ai())) << "NPCs should be close to each other";
}

TEST_F(AITest, testConditionIsOnCooldown) {
	const NpcPtr& npc = create();
	const backend::ConditionFactoryContext ctx(network::EnumNameCooldownType(cooldown::Type::INCREASE));
	const backend::ConditionPtr& condition = IsOnCooldown::getFactory().create(&ctx);
	EXPECT_EQ(cooldown::CooldownTriggerState::SUCCESS, npc->cooldownMgr().triggerCooldown(cooldown::Type::INCREASE));
	EXPECT_TRUE(condition->evaluate(npc->ai())) << "NPC should have the cooldown triggered";
}

TEST_F(AITest, testActionTriggerCooldown) {
	const NpcPtr& npc = create();
	const backend::TreeNodeFactoryContext ctx("foo", network::EnumNameCooldownType(cooldown::Type::INCREASE), backend::True::get());
	const TreeNodePtr& action = TriggerCooldown::getFactory().create(&ctx);
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	EXPECT_TRUE(npc->cooldownMgr().isCooldown(cooldown::Type::INCREASE));
}

TEST_F(AITest, testActionTriggerCooldownOnSelection) {
	const NpcPtr& npc = create();
	setVisible(npc);
	const backend::TreeNodeFactoryContext ctx("foo", network::EnumNameCooldownType(cooldown::Type::INCREASE), backend::True::get());
	const TreeNodePtr& action = TriggerCooldown::getFactory().create(&ctx);
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
}

TEST_F(AITest, testActionSpawn) {
	const NpcPtr& npc = create();
	const backend::TreeNodeFactoryContext ctx("foo", "", backend::True::get());
	const TreeNodePtr& action = Spawn::getFactory().create(&ctx);
	const int before = map->npcCount();
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	const int after = map->npcCount();
	EXPECT_EQ(before + 1, after) << "NPC wasn't spawned as expected";
}

TEST_F(AITest, testActionSetPointOfInterest) {
	const NpcPtr& npc = create();
	const size_t before = map->poiProvider().count();
	const backend::TreeNodeFactoryContext ctx("foo", "", backend::True::get());
	const TreeNodePtr& action = SetPointOfInterest::getFactory().create(&ctx);
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	EXPECT_GT(map->poiProvider().count(), before);
}

TEST_F(AITest, testActionGoHome) {
	const NpcPtr& npc = create();
	const backend::TreeNodeFactoryContext ctx("foo", "", backend::True::get());
	const TreeNodePtr& action = GoHome::getFactory().create(&ctx);
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
}

TEST_F(AITest, testActionDie) {
	const NpcPtr& npc = create();
	const backend::TreeNodeFactoryContext ctx("foo", "", backend::True::get());
	const TreeNodePtr& action = Die::getFactory().create(&ctx);
	EXPECT_FALSE(npc->dead()) << "NPC should be alive";
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	EXPECT_TRUE(npc->dead()) << "NPC should be dead";
}

TEST_F(AITest, testActionAttackOnSelection) {
	const NpcPtr& npc = create();
	const backend::TreeNodeFactoryContext ctx("foo", "", backend::True::get());
	const TreeNodePtr& action = AttackOnSelection::getFactory().create(&ctx);
	EXPECT_EQ(ai::TreeNodeStatus::FAILED, action->execute(npc->ai(), 0L));
	setVisible(npc);
	EXPECT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
}

TEST_F(AITest, testLast) {
	const NpcPtr& npc = create();
	Filters filters;
	filters.push_back(std::make_shared<FakeFilter>("3,9,10,2,1", Filters()));
	filters.push_back(std::make_shared<FakeFilter>("3,10,2,4", Filters()));
	backend::Last filter("", filters);
	filter.filter(npc->ai());
	const FilteredEntities& filteredEntities = npc->ai()->getFilteredEntities();
	ASSERT_GE(filteredEntities.size(), 1u);
	EXPECT_EQ(1u, filteredEntities.size());
	EXPECT_EQ(4, filteredEntities[0]);
}

TEST_F(AITest, testFirst) {
	const NpcPtr& npc = create();
	Filters filters;
	filters.push_back(std::make_shared<FakeFilter>("11,2,3", Filters()));
	filters.push_back(std::make_shared<FakeFilter>("3,10,4", Filters()));
	backend::First filter("", filters);
	filter.filter(npc->ai());
	const FilteredEntities& filteredEntities = npc->ai()->getFilteredEntities();
	ASSERT_GE(filteredEntities.size(), 1u);
	EXPECT_EQ(1u, filteredEntities.size());
	EXPECT_EQ(11, filteredEntities[0]);
}

TEST_F(AITest, testIntersection) {
	const NpcPtr& npc = create();
	Filters filters;
	filters.push_back(std::make_shared<FakeFilter>("1,2,3,5,6,10,4", Filters()));
	filters.push_back(std::make_shared<FakeFilter>("3,9,10,2,4", Filters()));
	filters.push_back(std::make_shared<FakeFilter>("3,10,2,4", Filters()));
	backend::Intersection filter("", filters);
	filter.filter(npc->ai());
	const FilteredEntities& filteredEntities = npc->ai()->getFilteredEntities();
	ASSERT_GE(filteredEntities.size(), 3u);
	EXPECT_EQ(4u, filteredEntities.size());
	EXPECT_EQ(2, filteredEntities[0]);
	EXPECT_EQ(3, filteredEntities[1]);
	EXPECT_EQ(4, filteredEntities[2]);
	EXPECT_EQ(10, filteredEntities[3]);
}

TEST_F(AITest, testDifference) {
	const NpcPtr& npc = create();
	Filters filters;
	filters.push_back(std::make_shared<FakeFilter>("1,2,3,5,6,10,4", Filters()));
	filters.push_back(std::make_shared<FakeFilter>("3,9,10,2,4", Filters()));
	backend::Difference filter("", filters);
	filter.filter(npc->ai());
	const FilteredEntities& filteredEntities = npc->ai()->getFilteredEntities();
	ASSERT_GE(filteredEntities.size(), 3u);
	EXPECT_EQ(3u, filteredEntities.size());
	EXPECT_EQ(1, filteredEntities[0]);
	EXPECT_EQ(5, filteredEntities[1]);
	EXPECT_EQ(6, filteredEntities[2]);
}

TEST_F(AITest, testComplement) {
	const NpcPtr& npc = create();
	Filters filters;
	npc->ai()->addFilteredEntity(1);
	npc->ai()->addFilteredEntity(2);
	npc->ai()->addFilteredEntity(4);
	npc->ai()->addFilteredEntity(19);
	filters.push_back(std::make_shared<FakeFilter>("1,2,3,5,6,10,4", Filters()));
	filters.push_back(std::make_shared<FakeFilter>("3,9,10,2,4", Filters()));
	backend::Complement filter("", filters);
	filter.filter(npc->ai());
	const FilteredEntities& filteredEntities = npc->ai()->getFilteredEntities();
	ASSERT_GE(filteredEntities.size(), 1u);
	EXPECT_EQ(1u, filteredEntities.size());
	EXPECT_EQ(19, filteredEntities[0]);
}

TEST_F(AITest, testUnion) {
	const NpcPtr& npc = create();
	Filters filters;
	filters.push_back(std::make_shared<FakeFilter>("1,2,3,5,6,10,4", Filters()));
	filters.push_back(std::make_shared<FakeFilter>("3,9,10,2,4", Filters()));
	backend::Union filter("", filters);
	filter.filter(npc->ai());
	const FilteredEntities& filteredEntities = npc->ai()->getFilteredEntities();
	ASSERT_GE(filteredEntities.size(), 8u);
	EXPECT_EQ(8u, filteredEntities.size());
	EXPECT_EQ(1, filteredEntities[0]);
	EXPECT_EQ(2, filteredEntities[1]);
	EXPECT_EQ(3, filteredEntities[2]);
	EXPECT_EQ(4, filteredEntities[3]);
	EXPECT_EQ(5, filteredEntities[4]);
	EXPECT_EQ(6, filteredEntities[5]);
	EXPECT_EQ(9, filteredEntities[6]);
	EXPECT_EQ(10, filteredEntities[7]);
}

}
