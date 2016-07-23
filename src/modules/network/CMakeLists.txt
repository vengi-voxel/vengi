set(MESSAGE_OUT_DIR ${CMAKE_BINARY_DIR}/messages)

macro(network_generate HEADER DEFINITION)
	add_custom_command(
		OUTPUT ${MESSAGE_OUT_DIR}/${HEADER}
		COMMAND ${CMAKE_BINARY_DIR}/flatc -c --gen-includes -o ${MESSAGE_OUT_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/definitions/${DEFINITION}
		DEPENDS flatc ${CMAKE_CURRENT_SOURCE_DIR}/definitions/${DEFINITION}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating source code for ${DEFINITION}"
	)
	set_source_files_properties(${MESSAGE_OUT_DIR}/${HEADER} PROPERTIES GENERATED TRUE)
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
	DEPENDS ${MESSAGE_OUT_DIR}/Shared_generated.h ${MESSAGE_OUT_DIR}/ClientMessages_generated.h ${MESSAGE_OUT_DIR}/ServerMessages_generated.h
	COMMENT "Generate network messages in ${MESSAGE_OUT_DIR}"
)

network_generate(Shared_generated.h Shared.fbs)
network_generate(ClientMessages_generated.h ClientMessages.fbs)
network_generate(ServerMessages_generated.h ServerMessages.fbs)

target_link_libraries(${LIB} core enet flatbuffers)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
add_dependencies(${LIB} GenerateNetworkMessages)
