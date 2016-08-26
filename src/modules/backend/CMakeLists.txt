set(SRCS
	spawn/SpawnMgr.cpp spawn/SpawnMgr.h
	loop/ServerLoop.cpp loop/ServerLoop.h
	entity/ai/AICharacter.cpp entity/ai/AICharacter.h
	entity/ai/AIRegistry.cpp entity/ai/AIRegistry.h
	entity/ai/AILoader.h
	network/UserConnectHandler.cpp network/UserConnectHandler.h
	network/UserConnectedHandler.h
	network/UserDisconnectHandler.h
	network/AttackHandler.h
	network/MoveHandler.h
	network/IUserProtocolHandler.h
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
	entity/ai/filter/SelectNpcsOfTypes.h
	poi/PoiProvider.cpp poi/PoiProvider.h
	entity/Npc.cpp entity/Npc.h
	entity/User.cpp entity/User.h
	entity/EntityId.h
	entity/EntityStorage.cpp entity/EntityStorage.h
	entity/Entity.cpp entity/Entity.h
)
set(LIB backend)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} network io core util voxel attrib cooldown persistence ai)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
generate_db_models(${LIB} ${ROOT_DIR}/data/server/database/tables.tbl DatabaseModels.h)

gtest_suite_files(tests
	tests/DatabaseModelTest.cpp
	tests/SpawnMgrTest.cpp
	tests/PoiProviderTest.cpp
)
gtest_suite_deps(tests ${LIB})
