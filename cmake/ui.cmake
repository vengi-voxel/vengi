macro(check_ui_turbobadger TARGET)
	if (NOT CMAKE_CROSS_COMPILING)
		set(_files ${ARGN})
		set(_workingdir "${DATA_DIR}/${TARGET}")
		foreach(_file ${_files})
			get_filename_component(_filename ${_file} NAME)
			add_custom_target(
				${TARGET}-${_filename}
				COMMAND uitool ../${_file}
				DEPENDS uitool ${DATA_DIR}/${_file}
				WORKING_DIRECTORY ${_workingdir}
				SOURCES ${DATA_DIR}/${_file}
			)
			add_dependencies(${TARGET} ${TARGET}-${_filename})
		endforeach()
	endif()
endmacro()
