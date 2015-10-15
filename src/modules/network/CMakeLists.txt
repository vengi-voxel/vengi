fips_begin_module(network)
	fips_files(
		IProtocolHandler.h
		IMsgProtocolHandler.h
		Network.cpp Network.h
		MessageSender.h MessageSender.cpp
		NetworkEvents.h
		ProtocolHandlerRegistry.h
	)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	fips_deps(core enet ${SDL2_LIBRARIES})
	fips_generate(TYPE FlatC FROM definitions/Shared.fbs HEADER ../messages/Shared_generated.h)
	fips_generate(TYPE FlatC FROM definitions/ClientMessages.fbs HEADER ../messages/ClientMessages.h)
	fips_generate(TYPE FlatC FROM definitions/ServerMessages.fbs HEADER ../messages/ServerMessages.h)
fips_end_module()
