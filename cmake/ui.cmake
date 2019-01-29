macro(check_ui_turbobadger TARGET)
	set(_workingdir "${DATA_DIR}/${TARGET}")
	set(_dir "${_workingdir}/ui/window")
	file(GLOB UI_FILES ${_dir}/*.tb.txt)
	foreach(_file ${UI_FILES})
		get_filename_component(_filename ${_file} NAME)
		add_custom_target(
			${_filename}
			COMMAND uitool ui/window/${_filename}
			COMMENT "Validate ui file: ${_filename}"
			DEPENDS uitool
			WORKING_DIRECTORY ${_workingdir}
		)
		add_dependencies(${TARGET} ${_filename})
	endforeach()
	if (UI_FILES)
		#add_dependencies(${TARGET} uitool)
	endif()
endmacro()
