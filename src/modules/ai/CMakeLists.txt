set(SRCS
	aggro/AggroMgr.h aggro/AggroMgr.cpp
	aggro/Entry.h
	AI.h AI.cpp
	AIFactories.h
	AIRegistry.h AIRegistry.cpp
	LUAAIRegistry.h LUAAIRegistry.cpp
	LUAFunctions.h LUAFunctions.cpp
	common/IFactoryRegistry.h
	common/IParser.h
	common/Log.h
	common/Math.h
	common/MemoryAllocator.h
	common/MoveVector.h
	common/NonCopyable.h
	common/Random.h
	common/String.h
	common/Thread.h
	common/Types.h
	conditions/And.h
	conditions/ConditionParser.h
	conditions/ConditionParser.cpp
	conditions/False.h
	conditions/Filter.h
	conditions/Filter.cpp
	conditions/HasEnemies.h
	conditions/ICondition.h
	conditions/IsCloseToGroup.h
	conditions/IsGroupLeader.h
	conditions/IsInGroup.h
	conditions/Not.h
	conditions/Or.h
	conditions/True.h
	filter/IFilter.h
	filter/SelectEmpty.h
	filter/SelectGroupLeader.h
	filter/SelectGroupMembers.h
	filter/SelectHighestAggro.h
	filter/SelectZone.h
	filter/Union.h
	filter/Union.cpp
	filter/Intersection.h
	filter/First.h
	filter/First.cpp
	filter/Last.h
	filter/Last.cpp
	filter/Random.h
	filter/Random.cpp
	filter/Difference.h
	filter/Difference.cpp
	filter/Complement.h
	filter/Complement.cpp
	filter/SelectAll.h
	IAIFactory.h
	ICharacter.h
	group/GroupId.h
	group/GroupMgr.h group/GroupMgr.cpp
	movement/SelectionSeek.h
	movement/GroupFlee.h movement/GroupFlee.cpp
	movement/GroupSeek.h movement/GroupSeek.cpp
	movement/Steering.h
	movement/TargetFlee.h
	movement/TargetSeek.h
	movement/Wander.h movement/Wander.cpp
	movement/WeightedSteering.h
	movement/LUASteering.h
	movement/LUASteering.cpp
	server/AIAddNodeMessage.h
	server/AIChangeMessage.h
	server/AICharacterDetailsMessage.h server/AICharacterDetailsMessage.cpp
	server/AICharacterStaticMessage.h server/AICharacterStaticMessage.cpp
	server/AIDeleteNodeMessage.h
	server/AINamesMessage.h
	server/AIPauseMessage.h
	server/AISelectMessage.h
	server/AIStateMessage.h
	server/AIStepMessage.h
	server/AIStubTypes.h
	server/AIUpdateNodeMessage.h
	server/AddNodeHandler.h server/AddNodeHandler.cpp
	server/ChangeHandler.h server/ChangeHandler.cpp
	server/DeleteNodeHandler.h server/DeleteNodeHandler.cpp
	server/IProtocolHandler.h
	server/IProtocolMessage.h
	server/Network.h server/Network.cpp
	server/PauseHandler.h server/PauseHandler.cpp
	server/ProtocolHandlerRegistry.h
	server/ProtocolMessageFactory.h server/ProtocolMessageFactory.cpp
	server/ResetHandler.h server/ResetHandler.cpp
	server/SelectHandler.h server/SelectHandler.cpp
	server/Server.h server/Server.cpp
	server/StepHandler.h server/StepHandler.cpp
	server/UpdateNodeHandler.h server/UpdateNodeHandler.cpp
	zone/Zone.h zone/Zone.cpp
	SimpleAI.h
	tree/Fail.h
	tree/Limit.h
	tree/Idle.h
	tree/Invert.h
	tree/ITask.h
	tree/ITimedNode.h
	tree/loaders/ITreeLoader.h
	tree/Parallel.h
	tree/PrioritySelector.h
	tree/ProbabilitySelector.h
	tree/RandomSelector.h
	tree/Selector.h
	tree/Sequence.h
	tree/Steer.h
	tree/Succeed.h
	tree/TreeNode.h tree/TreeNode.cpp
	tree/TreeNodeParser.h tree/TreeNodeParser.cpp
	tree/loaders/lua/LUATreeLoader.h tree/loaders/lua/LUATreeLoader.cpp
)
set(LIB ai)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES commonlua)
target_include_directories(${LIB} PUBLIC .)
target_compile_definitions(${LIB} PUBLIC -DAI_INCLUDE_LUA=1)

set(TEST_SRCS
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
gtest_suite_files(tests tests/testluaregistry.lua)
gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_files(tests-${LIB} tests/testluaregistry.lua)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
