macro(generate_compute_shaders TARGET)
	set(files ${ARGV})
	list(REMOVE_AT files 0)
	set(GEN_DIR ${GENERATE_DIR}/compute-shaders/${TARGET}/)
	set(_template ${ROOT_DIR}/src/tools/computeshadertool/ComputeShaderTemplate.h.in)
	file(MAKE_DIRECTORY ${GEN_DIR})
	target_include_directories(${TARGET} PUBLIC ${GEN_DIR})
	set(_headers)
	add_custom_target(UpdateComputeShaders${TARGET})
	file(WRITE ${CMAKE_BINARY_DIR}/GenerateComputeShaderHeader${TARGET}.cmake "configure_file(\${SRC} \${DST} @ONLY)")
	if (NOT DEFINED compute_SOURCE_DIR)
		message(FATAL_ERROR "compute project not found")
	endif()
	set(SHADERTOOL_INCLUDE_DIRS)
	list(APPEND SHADERTOOL_INCLUDE_DIRS "${compute_SOURCE_DIR}/shaders")
	get_property(DEPENDENCIES GLOBAL PROPERTY ${TARGET}_DEPENDENCIES)
	foreach (D ${DEPENDENCIES})
		if (NOT DEFINED ${D}_SOURCE_DIR)
			continue()
		endif()
		if (EXISTS ${${D}_SOURCE_DIR}/shaders)
			list(APPEND SHADERTOOL_INCLUDE_DIRS "${${D}_SOURCE_DIR}/shaders")
		endif()
	endforeach()
	list(REMOVE_DUPLICATES SHADERTOOL_INCLUDE_DIRS)
	set(SHADERTOOL_INCLUDE_DIRS_PARAM)
	foreach (IDIR ${SHADERTOOL_INCLUDE_DIRS})
		list(APPEND SHADERTOOL_INCLUDE_DIRS_PARAM "-I")
		list(APPEND SHADERTOOL_INCLUDE_DIRS_PARAM "${IDIR}")
	endforeach()
	foreach (_file ${files})
		set(_shaders)
		set(_dir ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
		if (EXISTS ${_dir}/${_file}.cl)
			list(APPEND _shaders ${_dir}/${_file}.cl)
		endif()
		if (_shaders)
			convert_to_camel_case(${_file} _f)
			set(_shaderfile "${_f}Shader.h")
			set(_shader "${GEN_DIR}${_shaderfile}")
			# TODO We have to add the shader/ dirs of all dependencies to the include path
			add_custom_command(
				OUTPUT ${_shader}.in
				IMPLICIT_DEPENDS C ${_shaders}
				COMMENT "Validate ${_file} and generate ${_shaderfile}"
				COMMAND computeshadertool --shader ${_dir}/${_file} -I ${_dir} ${SHADERTOOL_INCLUDE_DIRS_PARAM} --postfix .in --shadertemplate ${_template} --sourcedir ${GEN_DIR}
				DEPENDS computeshadertool ${_shaders} ${_template}
				VERBATIM
			)
			list(APPEND _headers ${_shader})
			add_custom_command(
				OUTPUT ${_shader}
				COMMAND ${CMAKE_COMMAND} -D SRC=${_shader}.in -D DST=${_shader} -P ${CMAKE_BINARY_DIR}/GenerateComputeShaderHeader${TARGET}.cmake
				DEPENDS ${_shader}.in
			)
		else()
			message(FATAL_ERROR "Could not find any shader files for ${_file} and target '${TARGET}'")
		endif()
	endforeach()

	convert_to_camel_case(${TARGET} _filetarget)
	set(_h ${GEN_DIR}/${_filetarget}ComputeShaders.h)
	file(WRITE ${_h}.in "#pragma once\n")
	foreach (header_path ${_headers})
		string(REPLACE "${GEN_DIR}" "" header "${header_path}")
		file(APPEND ${_h}.in "#include \"${header}\"\n")
	endforeach()
	add_custom_target(GenerateComputeShaderBindings${TARGET}
		DEPENDS ${_headers}
		COMMENT "Generate shader bindings for ${TARGET} in ${GEN_DIR}"
	)
	engine_mark_as_generated(${_headers} ${_h})
	add_custom_target(GenerateComputeShaderHeader${TARGET} ${CMAKE_COMMAND} -D SRC=${_h}.in -D DST=${_h} -P ${CMAKE_BINARY_DIR}/GenerateComputeShaderHeader${TARGET}.cmake)
	add_dependencies(${TARGET} GenerateComputeShaderHeader${TARGET} UpdateComputeShaders${TARGET})
	add_dependencies(GenerateComputeShaderHeader${TARGET} GenerateComputeShaderBindings${TARGET})
	add_dependencies(codegen GenerateComputeShaderHeader${TARGET} UpdateComputeShaders${TARGET})
endmacro()
