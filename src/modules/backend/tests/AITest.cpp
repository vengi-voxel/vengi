/**
 * @file
 */

#include "core/tests/AbstractTest.h"
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
#include "backend/entity/Npc.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/world/MapProvider.h"
#include "backend/world/Map.h"
#include "backend/spawn/SpawnMgr.h"
#include "poi/PoiProvider.h"
#include "network/ProtocolHandlerRegistry.h"
#include "network/ServerNetwork.h"
#include "network/ServerMessageSender.h"
#include "cooldown/CooldownProvider.h"
#include "attrib/ContainerProvider.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/AILoader.h"
#include "backend/entity/EntityStorage.h"
#include "voxel/MaterialColor.h"

namespace backend {

namespace {

const char *CONTAINER = R"(function init()
local rabbit = attrib.createContainer("ANIMAL_RABBIT")
rabbit:absolute("FIELDOFVIEW", 360.0)
rabbit:absolute("HEALTH", 100.0)
rabbit:absolute("STRENGTH", 1.0)
rabbit:absolute("VIEWDISTANCE", 10000.0)
rabbit:register()

local wolf = attrib.createContainer("ANIMAL_WOLF")
wolf:absolute("FIELDOFVIEW", 360.0)
wolf:absolute("HEALTH", 100.0)
wolf:absolute("STRENGTH", 1.0)
wolf:absolute("VIEWDISTANCE", 10000.0)
wolf:register()
end)";

}

class AITest: public core::AbstractTest {
private:
	using Super = core::AbstractTest;
protected:
	EntityStoragePtr entityStorage;
	network::ProtocolHandlerRegistryPtr protocolHandlerRegistry;
	network::ServerNetworkPtr network;
	network::ServerMessageSenderPtr messageSender;
	AIRegistryPtr registry;
	AILoaderPtr loader;
	attrib::ContainerProviderPtr containerProvider;
	cooldown::CooldownProviderPtr cooldownProvider;
	core::EventBusPtr eventBus;
	io::FilesystemPtr filesystem;
	core::TimeProviderPtr timeProvider;
	MapProviderPtr mapProvider;
	MapPtr map;

	void SetUp() override {
		Super::SetUp();
		core::Var::get(cfg::ServerSeed, "1");
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		voxel::initDefaultMaterialColors();
		entityStorage = std::make_shared<EntityStorage>(_testApp->eventBus());
		protocolHandlerRegistry = std::make_shared<network::ProtocolHandlerRegistry>();
		network = std::make_shared<network::ServerNetwork>(protocolHandlerRegistry, _testApp->eventBus());
		messageSender = std::make_shared<network::ServerMessageSender>(network);
		registry = std::make_shared<AIRegistry>();
		registry->init();
		loader = std::make_shared<AILoader>(registry);
		containerProvider = std::make_shared<attrib::ContainerProvider>();
		ASSERT_TRUE(containerProvider->init(CONTAINER));
		cooldownProvider = std::make_shared<cooldown::CooldownProvider>();
		filesystem = _testApp->filesystem();
		eventBus = _testApp->eventBus();
		timeProvider = _testApp->timeProvider();
		mapProvider = std::make_shared<MapProvider>(filesystem, eventBus, timeProvider,
				entityStorage, messageSender, loader, containerProvider, cooldownProvider);
		ASSERT_TRUE(mapProvider->init()) << "Failed to initialize the map provider";
		map = mapProvider->map(1);
	}

	NpcPtr setVisible(const NpcPtr& npc) {
		const NpcPtr& partner = create();
		npc->updateVisible({partner});

		const ai::FilterFactoryContext filterCtx(network::EnumNameEntityType(partner->entityType()));
		const ai::FilterPtr& filter = SelectEntitiesOfTypes::getFactory().create(&filterCtx);
		filter->filter(npc->ai());
		return partner;
	}

	inline NpcPtr create(network::EntityType type = network::EntityType::ANIMAL_RABBIT) {
		constexpr glm::ivec3 pos(0);
		const NpcPtr& npc = map->spawnMgr()->spawn(type, &pos);
		map->zone()->update(0L);
		return npc;
	}
};

TEST_F(AITest, testFilterSelectIncreasePartner) {
	const NpcPtr& npc = create();
	const NpcPtr& partner = setVisible(npc);
	ASSERT_EQ(partner->id(), npc->ai()->getFilteredEntities().front());
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

	const FilteredEntities& fe = npc->ai()->getFilteredEntities();
	ASSERT_EQ(2u, fe.size())
		<< "Expected to have two of the npcs visible, but " << fe.size() << " are";
	ASSERT_TRUE(std::find(fe.begin(), fe.end(), npcNotVisible->id()) == fe.end())
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

	const FilteredEntities& fe = npc->ai()->getFilteredEntities();
	ASSERT_EQ(2u, fe.size())
		<< "Expected to have two of the npcs visible, but " << fe.size() << " are";
}

TEST_F(AITest, testConditionIsSelectionAlive) {
	const NpcPtr& npc = create();
	setVisible(npc);
	const ai::ConditionFactoryContext ctx("");
	const ai::ConditionPtr& condition = IsSelectionAlive::getFactory().create(&ctx);
	ASSERT_TRUE(condition->evaluate(npc->ai())) << "NPC should be alive";
}

TEST_F(AITest, testConditionIsClosetoSelection) {
	const NpcPtr& npc = create();
	setVisible(npc);
	const ai::ConditionFactoryContext ctx("");
	const ai::ConditionPtr& condition = IsCloseToSelection::getFactory().create(&ctx);
	ASSERT_TRUE(condition->evaluate(npc->ai())) << "NPCs should be close to each other";
}

TEST_F(AITest, testConditionIsOnCooldown) {
	const NpcPtr& npc = create();
	const ai::ConditionFactoryContext ctx(network::EnumNameCooldownType(cooldown::Type::INCREASE));
	const ai::ConditionPtr& condition = IsOnCooldown::getFactory().create(&ctx);
	ASSERT_EQ(cooldown::CooldownTriggerState::SUCCESS, npc->cooldownMgr().triggerCooldown(cooldown::Type::INCREASE));
	ASSERT_TRUE(condition->evaluate(npc->ai())) << "NPC should have the cooldown triggered";
}

TEST_F(AITest, testActionTriggerCooldown) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", network::EnumNameCooldownType(cooldown::Type::INCREASE), ai::True::get());
	const ai::TreeNodePtr& action = TriggerCooldown::getFactory().create(&ctx);
	ASSERT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	ASSERT_TRUE(npc->cooldownMgr().isCooldown(cooldown::Type::INCREASE));
}

TEST_F(AITest, testActionTriggerCooldownOnSelection) {
	const NpcPtr& npc = create();
	setVisible(npc);
	const ai::TreeNodeFactoryContext ctx("foo", network::EnumNameCooldownType(cooldown::Type::INCREASE), ai::True::get());
	const ai::TreeNodePtr& action = TriggerCooldown::getFactory().create(&ctx);
	ASSERT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
}

TEST_F(AITest, testActionSpawn) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", "", ai::True::get());
	const ai::TreeNodePtr& action = Spawn::getFactory().create(&ctx);
	const int before = map->npcCount();
	ASSERT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	const int after = map->npcCount();
	ASSERT_EQ(before + 1, after) << "NPC wasn't spawned as expected";
}

TEST_F(AITest, testActionSetPointOfInterest) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", "", ai::True::get());
	const ai::TreeNodePtr& action = SetPointOfInterest::getFactory().create(&ctx);
	ASSERT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	ASSERT_EQ(1u, map->poiProvider()->getPointOfInterestCount());
}

TEST_F(AITest, testActionGoHome) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", "", ai::True::get());
	const ai::TreeNodePtr& action = GoHome::getFactory().create(&ctx);
	ASSERT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
}

TEST_F(AITest, testActionDie) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", "", ai::True::get());
	const ai::TreeNodePtr& action = Die::getFactory().create(&ctx);
	ASSERT_FALSE(npc->dead()) << "NPC should be alive";
	ASSERT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
	ASSERT_TRUE(npc->dead()) << "NPC should be dead";
}

TEST_F(AITest, testActionAttackOnSelection) {
	const NpcPtr& npc = create();
	const ai::TreeNodeFactoryContext ctx("foo", "", ai::True::get());
	const ai::TreeNodePtr& action = AttackOnSelection::getFactory().create(&ctx);
	ASSERT_EQ(ai::TreeNodeStatus::FAILED, action->execute(npc->ai(), 0L));
	setVisible(npc);
	ASSERT_EQ(ai::TreeNodeStatus::FINISHED, action->execute(npc->ai(), 0L));
}

}
