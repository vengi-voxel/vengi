set(SRCS
	ClientMessageSender.h ClientMessageSender.cpp
	ClientNetwork.h ClientNetwork.cpp
	IProtocolHandler.h
	IMsgProtocolHandler.h
	Network.cpp Network.h
	NetworkEvents.h
	ProtocolEnum.h
	ProtocolHandlerRegistry.h ProtocolHandlerRegistry.cpp
	ServerMessageSender.h ServerMessageSender.cpp
	ServerNetwork.h ServerNetwork.cpp
)
set(LIB network)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core libenet flatbuffers)
generate_protocol(${LIB} Shared.fbs ClientMessages.fbs ServerMessages.fbs)
