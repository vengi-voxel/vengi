set(SRCS
	ForwardDecl.h

	spawn/SpawnMgr.cpp spawn/SpawnMgr.h

	loop/ServerLoop.cpp loop/ServerLoop.h

	attack/AttackMgr.cpp attack/AttackMgr.h

	world/DBChunkPersister.h world/DBChunkPersister.cpp
	world/Map.cpp world/Map.h
	world/MapId.h
	world/MapProvider.cpp world/MapProvider.h
	world/World.cpp world/World.h

	network/IUserProtocolHandler.h
	network/MoveHandler.h
	network/TriggerActionHandler.h
	network/UserConnectHandler.cpp network/UserConnectHandler.h
	network/UserConnectedHandler.h
	network/UserDisconnectHandler.h
	network/VarUpdateHandler.h

	metric/MetricMgr.cpp metric/MetricMgr.h

	entity/ai/AICharacter.h
	entity/ai/AILoader.h
	entity/ai/action/GoHome.cpp entity/ai/action/GoHome.h
	entity/ai/action/Spawn.cpp entity/ai/action/Spawn.h
	entity/ai/action/Die.cpp entity/ai/action/Die.h
	entity/ai/action/SetPointOfInterest.cpp entity/ai/action/SetPointOfInterest.h
	entity/ai/action/AttackOnSelection.cpp entity/ai/action/AttackOnSelection.h
	entity/ai/action/TriggerCooldown.cpp entity/ai/action/TriggerCooldown.h
	entity/ai/action/TriggerCooldownOnSelection.cpp entity/ai/action/TriggerCooldownOnSelection.h
	entity/ai/filter/SelectVisible.cpp entity/ai/filter/SelectVisible.h
	entity/ai/filter/SelectIncreasePartner.cpp entity/ai/filter/SelectIncreasePartner.h
	entity/ai/filter/SelectEntitiesOfTypes.h entity/ai/filter/SelectEntitiesOfTypes.cpp
	entity/ai/movement/WanderAroundHome.h entity/ai/movement/WanderAroundHome.cpp

	entity/ai/common/Common.h
	entity/ai/common/IFactoryRegistry.h
	entity/ai/common/IParser.h
	entity/ai/common/IParser.cpp
	entity/ai/common/Math.h entity/ai/common/Math.cpp
	entity/ai/common/MemoryAllocator.h
	entity/ai/common/MoveVector.h
	entity/ai/common/MoveVector.cpp
	entity/ai/common/Random.h

	entity/ai/aggro/AggroMgr.h entity/ai/aggro/AggroMgr.cpp
	entity/ai/aggro/Entry.h
	entity/ai/AI.h entity/ai/AI.cpp
	entity/ai/AIFactories.h
	entity/ai/AIRegistry.h entity/ai/AIRegistry.cpp
	entity/ai/LUAAIRegistry.h entity/ai/LUAAIRegistry.cpp
	entity/ai/LUAFunctions.h entity/ai/LUAFunctions.cpp
	entity/ai/condition/And.h
	entity/ai/condition/ConditionParser.h
	entity/ai/condition/ConditionParser.cpp
	entity/ai/condition/False.h
	entity/ai/condition/Filter.h
	entity/ai/condition/Filter.cpp
	entity/ai/condition/HasEnemies.h
	entity/ai/condition/HasEnemies.cpp
	entity/ai/condition/ICondition.h
	entity/ai/condition/IsCloseToGroup.h
	entity/ai/condition/IsCloseToGroup.cpp
	entity/ai/condition/IsCloseToSelection.h
	entity/ai/condition/IsCloseToSelection.cpp
	entity/ai/condition/IsGroupLeader.h
	entity/ai/condition/IsGroupLeader.cpp
	entity/ai/condition/IsInGroup.h
	entity/ai/condition/IsInGroup.cpp
	entity/ai/condition/IsOnCooldown.h
	entity/ai/condition/IsOnCooldown.cpp
	entity/ai/condition/IsSelectionAlive.h
	entity/ai/condition/IsSelectionAlive.cpp
	entity/ai/condition/LUACondition.h
	entity/ai/condition/LUACondition.cpp
	entity/ai/condition/Not.h
	entity/ai/condition/Or.h
	entity/ai/condition/True.h
	entity/ai/filter/IFilter.cpp
	entity/ai/filter/IFilter.h
	entity/ai/filter/FilteredEntities.h
	entity/ai/filter/SelectEmpty.h
	entity/ai/filter/SelectGroupLeader.cpp
	entity/ai/filter/SelectGroupLeader.h
	entity/ai/filter/SelectGroupMembers.cpp
	entity/ai/filter/SelectGroupMembers.h
	entity/ai/filter/SelectHighestAggro.cpp
	entity/ai/filter/SelectHighestAggro.h
	entity/ai/filter/SelectZone.cpp
	entity/ai/filter/SelectZone.h
	entity/ai/filter/Union.h
	entity/ai/filter/Union.cpp
	entity/ai/filter/Intersection.h
	entity/ai/filter/Intersection.cpp
	entity/ai/filter/First.h
	entity/ai/filter/First.cpp
	entity/ai/filter/Last.h
	entity/ai/filter/Last.cpp
	entity/ai/filter/LUAFilter.h
	entity/ai/filter/LUAFilter.cpp
	entity/ai/filter/Random.h
	entity/ai/filter/Random.cpp
	entity/ai/filter/Difference.h
	entity/ai/filter/Difference.cpp
	entity/ai/filter/Complement.h
	entity/ai/filter/Complement.cpp
	entity/ai/filter/SelectAll.h
	entity/ai/IAIFactory.h
	entity/ai/ICharacter.cpp
	entity/ai/ICharacter.h
	entity/ai/group/GroupId.h
	entity/ai/group/GroupMgr.h entity/ai/group/GroupMgr.cpp
	entity/ai/movement/SelectionSeek.h
	entity/ai/movement/SelectionSeek.cpp
	entity/ai/movement/SelectionFlee.h
	entity/ai/movement/SelectionFlee.cpp
	entity/ai/movement/GroupFlee.h entity/ai/movement/GroupFlee.cpp
	entity/ai/movement/GroupSeek.h entity/ai/movement/GroupSeek.cpp
	entity/ai/movement/Steering.h
	entity/ai/movement/Steering.cpp
	entity/ai/movement/TargetFlee.h
	entity/ai/movement/TargetFlee.cpp
	entity/ai/movement/TargetSeek.h
	entity/ai/movement/TargetSeek.cpp
	entity/ai/movement/Wander.h entity/ai/movement/Wander.cpp
	entity/ai/movement/WeightedSteering.cpp
	entity/ai/movement/WeightedSteering.h
	entity/ai/movement/LUASteering.h
	entity/ai/movement/LUASteering.cpp
	entity/ai/server/AddNodeHandler.h entity/ai/server/AddNodeHandler.cpp
	entity/ai/server/ChangeHandler.h entity/ai/server/ChangeHandler.cpp
	entity/ai/server/DeleteNodeHandler.h entity/ai/server/DeleteNodeHandler.cpp
	entity/ai/server/Network.h entity/ai/server/Network.cpp
	entity/ai/server/PauseHandler.h entity/ai/server/PauseHandler.cpp
	entity/ai/server/ResetHandler.h entity/ai/server/ResetHandler.cpp
	entity/ai/server/SelectHandler.h entity/ai/server/SelectHandler.cpp
	entity/ai/server/Server.h entity/ai/server/Server.cpp
	entity/ai/server/StepHandler.h entity/ai/server/StepHandler.cpp
	entity/ai/server/UpdateNodeHandler.h entity/ai/server/UpdateNodeHandler.cpp
	entity/ai/zone/Zone.h entity/ai/zone/Zone.cpp
	entity/ai/tree/Fail.cpp
	entity/ai/tree/Fail.h
	entity/ai/tree/Limit.cpp
	entity/ai/tree/Limit.h
	entity/ai/tree/Idle.h
	entity/ai/tree/Invert.h
	entity/ai/tree/Invert.cpp
	entity/ai/tree/ITask.h
	entity/ai/tree/ITimedNode.cpp
	entity/ai/tree/ITimedNode.h
	entity/ai/tree/loaders/ITreeLoader.h
	entity/ai/tree/Parallel.h
	entity/ai/tree/Parallel.cpp
	entity/ai/tree/PrioritySelector.h
	entity/ai/tree/PrioritySelector.cpp
	entity/ai/tree/ProbabilitySelector.h
	entity/ai/tree/ProbabilitySelector.cpp
	entity/ai/tree/RandomSelector.h
	entity/ai/tree/RandomSelector.cpp
	entity/ai/tree/Selector.h
	entity/ai/tree/Sequence.cpp
	entity/ai/tree/Sequence.h
	entity/ai/tree/Steer.cpp
	entity/ai/tree/Steer.h
	entity/ai/tree/Succeed.cpp
	entity/ai/tree/Succeed.h
	entity/ai/tree/LUATreeNode.h
	entity/ai/tree/LUATreeNode.cpp
	entity/ai/tree/TreeNode.h entity/ai/tree/TreeNode.cpp
	entity/ai/tree/TreeNodeParser.h entity/ai/tree/TreeNodeParser.cpp
	entity/ai/tree/loaders/lua/LUATreeLoader.h entity/ai/tree/loaders/lua/LUATreeLoader.cpp

	entity/user/UserAttribMgr.h entity/user/UserAttribMgr.cpp
	entity/user/UserStockMgr.h entity/user/UserStockMgr.cpp
	entity/user/UserCooldownMgr.h entity/user/UserCooldownMgr.cpp
	entity/user/UserLogoutMgr.h entity/user/UserLogoutMgr.cpp
	entity/user/UserMovementMgr.h entity/user/UserMovementMgr.cpp

	entity/Npc.cpp entity/Npc.h
	entity/User.cpp entity/User.h
	entity/EntityId.h
	entity/EntityStorage.cpp entity/EntityStorage.h
	entity/Entity.cpp entity/Entity.h
)
set(FILES
	shared/worldparams.lua
	shared/biomes.lua
)

set(LIB backend)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} FILES ${FILES} DEPENDENCIES eventmgr poi ai-shared stock shared http console)
generate_db_models(${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tables.tbl BackendModels.h)

target_include_directories(${LIB} PRIVATE entity/ai entity)

set(TEST_FILES
	tests/behaviourtrees.lua
	tests/behaviourtreenodes.lua
	tests/test-attributes.lua
	tests/testluaregistry.lua
)
set(TEST_SRCS
	tests/AITest.cpp
	tests/ConnectTest.cpp
	tests/UserCooldownMgrTest.cpp
	tests/MapProviderTest.cpp
	tests/MapTest.cpp
	tests/WorldTest.cpp
	tests/EntityTest.h
	tests/NpcTest.h
	tests/UserTest.h

	tests/AggroTest.cpp
	tests/GeneralTest.cpp
	tests/GroupTest.cpp
	tests/LUAAIRegistryTest.cpp
	tests/LUATreeLoaderTest.cpp
	tests/MessageTest.cpp
	tests/MovementTest.cpp
	tests/NodeTest.cpp
	tests/ParserTest.cpp
	tests/TestShared.cpp
	tests/ZoneTest.cpp
)
gtest_suite_files(tests ${TEST_FILES})
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
