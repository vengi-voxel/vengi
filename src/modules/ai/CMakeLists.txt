fips_begin_module(ai)
    fips_files(
        AI.cpp AI.h
        AIRegistry.cpp AIRegistry.h
        IAIFactory.h
        ICharacter.h
        SimpleAI.h
    )
    fips_dir(group)
    fips_files(
        GroupMgr.cpp GroupMgr.h
    )
    fips_dir(server)
    fips_files(
        AIUpdateNodeMessage.h
        AIAddNodeMessage.h
        AIDeleteNodeMessage.h
        AIChangeMessage.h
        AICharacterDetailsMessage.h
        AICharacterStaticMessage.h
        AINamesMessage.h
        AIPauseMessage.h
        AISelectMessage.h
        AIStateMessage.h
        AIStepMessage.h
        AIStubTypes.h
        ChangeHandler.cpp ChangeHandler.h
        IProtocolHandler.h
        IProtocolMessage.h
        Network.cpp Network.h
        PauseHandler.cpp PauseHandler.h
        ProtocolHandlerRegistry.h
        ProtocolMessageFactory.cpp ProtocolMessageFactory.h
        ResetHandler.cpp ResetHandler.h
        SelectHandler.cpp SelectHandler.h
        Server.cpp Server.h
        StepHandler.cpp StepHandler.h
        UpdateNodeHandler.cpp UpdateNodeHandler.h
        DeleteNodeHandler.cpp DeleteNodeHandler.h
        AddNodeHandler.cpp AddNodeHandler.h
    )
    fips_dir(zone)
    fips_files(
        Zone.cpp Zone.h
    )
    fips_dir(filter)
    fips_files(
        IFilter.h
        SelectEmpty.cpp SelectEmpty.h
        SelectGroupLeader.cpp SelectGroupLeader.h
        SelectGroupMembers.cpp SelectGroupMembers.h
        SelectHighestAggro.cpp SelectHighestAggro.h
        SelectZone.cpp SelectZone.h
    )
    fips_dir(tree)
    fips_files(
        Fail.cpp Fail.h
        Idle.cpp Idle.h
        Invert.cpp Invert.h
        ITask.cpp ITask.h
        ITimedNode.cpp ITimedNode.h
        Limit.cpp Limit.h
        Parallel.cpp Parallel.h
        Print.cpp Print.h
        PrioritySelector.cpp PrioritySelector.h
        ProbabilitySelector.cpp ProbabilitySelector.h
        RandomSelector.cpp RandomSelector.h
        Selector.h
        Sequence.cpp Sequence.h
        Steer.cpp Steer.h
        Succeed.cpp Succeed.h
        TreeNode.cpp TreeNode.h
        TreeNodeParser.cpp TreeNodeParser.h
    )
    fips_dir(tree/loaders)
    fips_files(
        ITreeLoader.h
    )
    fips_dir(tree/loaders/lua)
    fips_files(
        LUACondition.h
        LUA.cpp LUA.h
        LUAFunctions.h
        LUANode.cpp LUANode.h
        LUATree.cpp LUATree.h
        LUATreeLoader.cpp LUATreeLoader.h
        LUACondition.h
    )
    fips_dir(conditions)
    fips_files(
        And.cpp And.h
        ConditionParser.cpp ConditionParser.h
        False.cpp False.h
        Filter.cpp Filter.h
        HasEnemies.cpp HasEnemies.h
        ICondition.cpp ICondition.h
        IsCloseToGroup.cpp IsCloseToGroup.h
        IsGroupLeader.cpp IsGroupLeader.h
        IsInGroup.cpp IsInGroup.h
        Not.cpp Not.h
        Or.cpp Or.h
        True.cpp True.h
    )
    fips_dir(common)
    fips_files(
        IFactoryRegistry.h
        IParser.h
        IPrintable.h
        Math.h
        MemoryAllocator.h
        MoveVector.h
        NonCopyable.h
        Random.h
        String.h
        Thread.h
        ThreadPool.h
        ThreadScheduler.h
        Types.h
    )
    fips_dir(movement)
    fips_files(
        GroupFlee.h
        GroupSeek.h
        SelectionFlee.h
        SelectionSeek.h
        Steering.cpp Steering.h
        TargetFlee.h
        TargetSeek.h
        Wander.h
        WeightedSteering.h
    )
    fips_dir(aggro)
    fips_files(
        AggroMgr.cpp AggroMgr.h
        Entry.h
    )
    fips_deps(lua53)
fips_end_module()
target_include_directories(ai PUBLIC .)
target_compile_definitions(ai PRIVATE -DAI_EXCEPTIONS=0)
