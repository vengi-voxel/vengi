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
add_library(${LIB} ${SRCS})

engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core libenet flatbuffers)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
generate_protocol(${LIB} Shared.fbs ClientMessages.fbs ServerMessages.fbs)
