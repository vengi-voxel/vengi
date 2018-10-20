macro(generate_db_models TARGET INPUT OUTPUT)
	set(GEN_DIR ${GENERATE_DIR}/dbmodels/${TARGET}/)
	file(MAKE_DIRECTORY ${GEN_DIR})
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
	add_custom_command(
		OUTPUT ${GEN_DIR}${OUTPUT}
		COMMENT "Generate ${OUTPUT}"
		COMMAND $<TARGET_FILE:databasetool> --tablefile ${INPUT} --outfile ${GEN_DIR}${OUTPUT}
		DEPENDS databasetool ${INPUT}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	)
	add_custom_target(GenerateDatabaseModelBindings${TARGET}
		DEPENDS ${GEN_DIR}${OUTPUT}
		COMMENT "Generate database model bindings for ${TARGET} in ${GEN_DIR}"
	)
	engine_mark_as_generated(${GEN_DIR}${OUTPUT})
	add_dependencies(${TARGET} GenerateDatabaseModelBindings${TARGET})
	add_dependencies(codegen GenerateDatabaseModelBindings${TARGET})
endmacro()
