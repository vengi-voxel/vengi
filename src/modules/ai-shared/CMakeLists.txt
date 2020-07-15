set(SRCS
	common/Assert.h
	common/CharacterId.h
	common/Common.h
	common/IFactoryRegistry.h
	common/IParser.h
	common/Log.h
	common/Math.h common/Math.cpp
	common/MemoryAllocator.h
	common/MoveVector.h
	common/NonCopyable.h
	common/Random.h
	common/StringUtil.h
	common/Thread.h

	protocol/AIAddNodeMessage.h
	protocol/AIChangeMessage.h
	protocol/AICharacterDetailsMessage.h protocol/AICharacterDetailsMessage.cpp
	protocol/AICharacterStaticMessage.h protocol/AICharacterStaticMessage.cpp
	protocol/AIDeleteNodeMessage.h
	protocol/AINamesMessage.h
	protocol/AIPauseMessage.h
	protocol/AISelectMessage.h
	protocol/AIStateMessage.h
	protocol/AIStepMessage.h
	protocol/AIStubTypes.h
	protocol/AIUpdateNodeMessage.h
	protocol/IProtocolHandler.h
	protocol/IProtocolMessage.h
	protocol/ProtocolHandlerRegistry.h
	protocol/ProtocolMessageFactory.h protocol/ProtocolMessageFactory.cpp
)
set(LIB ai-shared)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core)
