macro(network_generate HEADER OUTDIR DEFINITION)
	add_custom_command(
		OUTPUT ${OUTDIR}/${HEADER}
		COMMAND ${TOOLS_DIR}/flatc -c --gen-includes -o ${OUTDIR} ${CMAKE_CURRENT_SOURCE_DIR}/definitions/${DEFINITION}
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/definitions/${DEFINITION}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating source code for ${DEFINITION}"
	)
	set_source_files_properties(${OUTDIR}/${HEADER} PROPERTIES GENERATED TRUE)
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

set(MESSAGE_OUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/messages)

add_custom_target(GenerateNetworkMessages
	DEPENDS ${MESSAGE_OUT_DIR}/Shared_generated.h ${MESSAGE_OUT_DIR}/ClientMessages_generated.h ${MESSAGE_OUT_DIR}/ServerMessages_generated.h
	COMMENT "Generate network messages"
)

network_generate(Shared_generated.h ${MESSAGE_OUT_DIR} Shared.fbs)
network_generate(ClientMessages_generated.h ${MESSAGE_OUT_DIR} ClientMessages.fbs)
network_generate(ServerMessages_generated.h ${MESSAGE_OUT_DIR} ServerMessages.fbs)

target_link_libraries(${LIB} core enet flatbuffers)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
add_dependencies(${LIB} GenerateNetworkMessages)
