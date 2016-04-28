set(SRCS
	AI.cpp AI.h
	AIRegistry.cpp AIRegistry.h
	IAIFactory.h
	ICharacter.h
	SimpleAI.h
	group/GroupMgr.cpp group/GroupMgr.h
	server/AIUpdateNodeMessage.h
	server/AIAddNodeMessage.h
	server/AIDeleteNodeMessage.h
	server/AIChangeMessage.h
	server/AICharacterDetailsMessage.h
	server/AICharacterStaticMessage.h
	server/AINamesMessage.h
	server/AIPauseMessage.h
	server/AISelectMessage.h
	server/AIStateMessage.h
	server/AIStepMessage.h
	server/AIStubTypes.h
	server/ChangeHandler.cpp server/ChangeHandler.h
	server/IProtocolHandler.h
	server/IProtocolMessage.h
	server/Network.cpp server/Network.h
	server/PauseHandler.cpp server/PauseHandler.h
	server/ProtocolHandlerRegistry.h
	server/ProtocolMessageFactory.cpp server/ProtocolMessageFactory.h
	server/ResetHandler.cpp server/ResetHandler.h
	server/SelectHandler.cpp server/SelectHandler.h
	server/Server.cpp server/Server.h
	server/StepHandler.cpp server/StepHandler.h
	server/UpdateNodeHandler.cpp server/UpdateNodeHandler.h
	server/DeleteNodeHandler.cpp server/DeleteNodeHandler.h
	server/AddNodeHandler.cpp server/AddNodeHandler.h
	zone/Zone.cpp zone/Zone.h
	filter/IFilter.h
	filter/SelectEmpty.cpp filter/SelectEmpty.h
	filter/SelectGroupLeader.cpp filter/SelectGroupLeader.h
	filter/SelectGroupMembers.cpp filter/SelectGroupMembers.h
	filter/SelectHighestAggro.cpp filter/SelectHighestAggro.h
	filter/SelectZone.cpp filter/SelectZone.h
	tree/Fail.cpp tree/Fail.h
	tree/Idle.cpp tree/Idle.h
	tree/Invert.cpp tree/Invert.h
	tree/ITask.cpp tree/ITask.h
	tree/ITimedNode.cpp tree/ITimedNode.h
	tree/Limit.cpp tree/Limit.h
	tree/Parallel.cpp tree/Parallel.h
	tree/Print.cpp tree/Print.h
	tree/PrioritySelector.cpp tree/PrioritySelector.h
	tree/ProbabilitySelector.cpp tree/ProbabilitySelector.h
	tree/RandomSelector.cpp tree/RandomSelector.h
	tree/Selector.h
	tree/Sequence.cpp tree/Sequence.h
	tree/Steer.cpp tree/Steer.h
	tree/Succeed.cpp tree/Succeed.h
	tree/TreeNode.cpp tree/TreeNode.h
	tree/TreeNodeParser.cpp tree/TreeNodeParser.h
	tree/loaders/ITreeLoader.h
	tree/loaders/lua/LUACondition.h
	tree/loaders/lua/LUA.cpp tree/loaders/lua/LUA.h
	tree/loaders/lua/LUAFunctions.h
	tree/loaders/lua/LUANode.cpp tree/loaders/lua/LUANode.h
	tree/loaders/lua/LUATree.cpp tree/loaders/lua/LUATree.h
	tree/loaders/lua/LUATreeLoader.cpp tree/loaders/lua/LUATreeLoader.h
	tree/loaders/lua/LUACondition.h
	conditions/And.cpp conditions/And.h
	conditions/ConditionParser.cpp conditions/ConditionParser.h
	conditions/False.cpp conditions/False.h
	conditions/Filter.cpp conditions/Filter.h
	conditions/HasEnemies.cpp conditions/HasEnemies.h
	conditions/ICondition.cpp conditions/ICondition.h
	conditions/IsCloseToGroup.cpp conditions/IsCloseToGroup.h
	conditions/IsGroupLeader.cpp conditions/IsGroupLeader.h
	conditions/IsInGroup.cpp conditions/IsInGroup.h
	conditions/Not.cpp conditions/Not.h
	conditions/Or.cpp conditions/Or.h
	conditions/True.cpp conditions/True.h
	common/IFactoryRegistry.h
	common/IParser.h
	common/IPrintable.h
	common/Math.h
	common/MemoryAllocator.h
	common/MoveVector.h
	common/NonCopyable.h
	common/Random.h
	common/String.h
	common/Thread.h
	common/ThreadPool.h
	common/ThreadScheduler.h
	common/Types.h
	movement/GroupFlee.h
	movement/GroupSeek.h
	movement/SelectionFlee.h
	movement/SelectionSeek.h
	movement/Steering.cpp movement/Steering.h
	movement/TargetFlee.h
	movement/TargetSeek.h
	movement/Wander.h
	movement/WeightedSteering.h
	aggro/AggroMgr.cpp aggro/AggroMgr.h
	aggro/Entry.h
)
set(LIB ai)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} lua53)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_include_directories(${LIB} PUBLIC .)
target_compile_definitions(${LIB} PRIVATE -DAI_EXCEPTIONS=0)
