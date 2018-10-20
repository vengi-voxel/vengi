macro(generate_protocol TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	set(GEN_DIR ${GENERATE_DIR}/protocol/${TARGET}/)
	file(MAKE_DIRECTORY ${GEN_DIR})
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
	set(_headers)
	foreach (_file ${files})
		get_filename_component(_basefilename ${_file} NAME_WE)
		set(HEADER "${_basefilename}_generated.h")
		set(DEFINITION ${_file})
		list(APPEND _headers ${GEN_DIR}${HEADER})
		add_custom_command(
			OUTPUT ${GEN_DIR}${HEADER}
			COMMAND $<TARGET_FILE:flatc> -c -I ${CMAKE_CURRENT_SOURCE_DIR}/../attrib/definitions --scoped-enums -o ${GEN_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/definitions/${DEFINITION}
			DEPENDS flatc ${CMAKE_CURRENT_SOURCE_DIR}/definitions/${DEFINITION}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMENT "Generating source code for ${DEFINITION}"
		)
		engine_mark_as_generated(${GEN_DIR}/${HEADER})
	endforeach()

	add_custom_target(GenerateNetworkMessages${TARGET}
		DEPENDS ${_headers}
		COMMENT "Generate network messages in ${GEN_DIR}"
	)
	add_dependencies(${TARGET} GenerateNetworkMessages${TARGET})
	add_dependencies(codegen GenerateNetworkMessages${TARGET})
endmacro()
