set(SRCS
	common/CharacterAttributes.h
	common/CharacterId.h
	common/TreeNodeStatus.h

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
generate_protocol(${LIB} AIMessages.fbs)
