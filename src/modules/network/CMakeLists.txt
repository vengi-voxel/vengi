macro(network_generate HEADER OUTDIR DEFINITION)
	add_custom_command(
		OUTPUT ${OUTDIR}/${HEADER}
		COMMAND ${TOOLS_DIR}/flatc -c --gen-includes -o ${OUTDIR} definitions/${DEFINITION}
		DEPENDS definitions/${DEFINITION}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating source code for ${DEFINITION}"
	)
endmacro()

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

add_custom_target(GenerateNetworkMessages
	DEPENDS messages/Shared_generated.h messages/ClientMessages_generated.h messages/ServerMessages_generated.h
	COMMENT "Generate network messages"
)

network_generate(Shared_generated.h messages Shared.fbs)
network_generate(ClientMessages_generated.h messages ClientMessages.fbs)
network_generate(ServerMessages_generated.h messages ServerMessages.fbs)

target_link_libraries(${LIB} core enet flatbuffers)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
add_dependencies(${LIB} GenerateNetworkMessages)
