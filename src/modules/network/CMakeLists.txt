set(SRCS
	AbstractClientNetwork.h AbstractClientNetwork.cpp
	IProtocolHandler.h
	IMsgProtocolHandler.h
	Network.cpp Network.h
	NetworkEvents.h
	ProtocolHandlerRegistry.h ProtocolHandlerRegistry.cpp
)
set(LIB network)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core flatbuffers libenet)
