set(SRCS
	IProtocolHandler.h
	IMsgProtocolHandler.h
	Network.cpp Network.h
	MessageSender.h MessageSender.cpp
	NetworkEvents.h
	ProtocolHandlerRegistry.h
)
set(LIB network)
add_library(${LIB} ${SRCS})

target_link_libraries(${LIB} core enet flatbuffers restclient-cpp)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
generate_protocol(${LIB} Shared.fbs ClientMessages.fbs ServerMessages.fbs)
