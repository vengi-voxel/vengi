set(SRCS
	ForwardDecl.h

	spawn/SpawnMgr.cpp spawn/SpawnMgr.h

	loop/ServerLoop.cpp loop/ServerLoop.h

	attack/AttackMgr.cpp attack/AttackMgr.h

	world/Map.cpp world/Map.h
	world/MapId.h
	world/LUAFunctions.h
	world/MapProvider.cpp world/MapProvider.h
	world/World.cpp world/World.h

	network/UserConnectHandler.cpp network/UserConnectHandler.h
	network/UserConnectedHandler.h
	network/UserDisconnectHandler.h
	network/AttackHandler.h
	network/MoveHandler.h
	network/IUserProtocolHandler.h

	metric/MetricMgr.cpp metric/MetricMgr.h

	entity/ai/AICharacter.cpp entity/ai/AICharacter.h
	entity/ai/AIRegistry.cpp entity/ai/AIRegistry.h
	entity/ai/AILoader.h
	entity/ai/condition/IsCloseToSelection.h
	entity/ai/condition/IsSelectionAlive.h
	entity/ai/condition/IsOnCooldown.h
	entity/ai/action/Task.h
	entity/ai/action/GoHome.h
	entity/ai/action/Spawn.h
	entity/ai/action/Die.h
	entity/ai/action/SetPointOfInterest.h
	entity/ai/action/AttackOnSelection.h
	entity/ai/action/TriggerCooldown.cpp entity/ai/action/TriggerCooldown.h
	entity/ai/action/TriggerCooldownOnSelection.cpp entity/ai/action/TriggerCooldownOnSelection.h
	entity/ai/filter/SelectPrey.cpp entity/ai/filter/SelectPrey.h
	entity/ai/filter/SelectVisible.cpp entity/ai/filter/SelectVisible.h
	entity/ai/filter/SelectIncreasePartner.cpp entity/ai/filter/SelectIncreasePartner.h
	entity/ai/filter/SelectEntitiesOfTypes.h entity/ai/filter/SelectEntitiesOfTypes.cpp

	entity/user/UserAttribMgr.h entity/user/UserAttribMgr.cpp
	entity/user/UserStockMgr.h entity/user/UserStockMgr.cpp
	entity/user/UserCooldownMgr.h entity/user/UserCooldownMgr.cpp

	entity/Npc.cpp entity/Npc.h
	entity/User.cpp entity/User.h
	entity/EntityId.h
	entity/EntityStorage.cpp entity/EntityStorage.h
	entity/Entity.cpp entity/Entity.h
)
set(LIB backend)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES network io core metric util voxel attrib eventmgr poi cooldown persistence ai stock util)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
generate_db_models(${LIB} ${CMAKE_CURRENT_SOURCE_DIR}/tables.tbl BackendModels.h)

gtest_suite_files(tests
	tests/DatabaseModelTest.cpp
	tests/MapProviderTest.cpp
	tests/MapTest.cpp
)
gtest_suite_deps(tests ${LIB})
test_generate_db_models(tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/tests.tbl TestModels.h)
