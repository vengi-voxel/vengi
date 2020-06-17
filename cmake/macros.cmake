function(engine_install TARGET FILE DESTINATION INSTALL_DATA)
	set(INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}-${TARGET}")
	get_filename_component(filename ${FILE} NAME)
	target_sources(${TARGET} PRIVATE ${DATA_DIR}/${FILE})
	set_source_files_properties(${DATA_DIR}/${FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${DESTINATION})
	if (INSTALL_DATA)
		install(FILES ${DATA_DIR}/${FILE} DESTINATION ${INSTALL_DATA_DIR}/${DESTINATION} COMPONENT ${TARGET})
	endif()
	configure_file(${DATA_DIR}/${FILE} ${CMAKE_BINARY_DIR}/${TARGET}/${DESTINATION}/${filename} COPYONLY)
endfunction()

#
# set up the binary for the application. This will also set up platform specific stuff for you
#
# Example: engine_add_executable(TARGET SomeTargetName SRCS Source.cpp Main.cpp WINDOWED)
#
# TARGET:    the target name (binary name)
# SRCS:      the source files for this target
# WINDOWED:  this is needed to indicate whether the application should e.g. spawn a console on windows
# NOINSTALL: means that the binary and data files are not put into the final installation folder
#            this can e.g. be useful for stuff like code generators that are only needed during build
#            time.
#
function(engine_add_executable)
	set(_OPTIONS_ARGS WINDOWED NOINSTALL)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS SRCS LUA_SRCS FILES)

	cmake_parse_arguments(_EXE "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	# e.g. used in desktop files
	set(COMMANDLINE "${CMAKE_PROJECT_NAME}-${_EXE_TARGET}")
	set(CATEGORIES "Game")
	set(DESCRIPTION "")
	set(NAME ${_EXE_TARGET})
	set(APPICON "${_EXE_TARGET}-icon")
	if (APPLE)
		set(ICON "${APPICON}.icns")
	elseif (WIN32)
		set(ICON "${APPICON}.ico")
	else()
		set(ICON "${APPICON}.png")
	endif()
	set(ICON_FULL_PATH ${DATA_DIR}/${_EXE_TARGET}/${ICON})
	if (EXISTS ${ICON_FULL_PATH})
		set(HAS_ICON 1)
	else()
		set(HAS_ICON 0)
	endif()

	set(${_EXE_TARGET}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "${_EXE_TARGET} source directory")
	if (_EXE_WINDOWED)
		generate_unity_sources(WINDOWED EXECUTABLE TARGET ${_EXE_TARGET} SRCS ${_EXE_SRCS})
	else()
		generate_unity_sources(EXECUTABLE TARGET ${_EXE_TARGET} SRCS ${_EXE_SRCS})
	endif()
	set_target_properties(${_EXE_TARGET} PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}-${_EXE_TARGET}")
	set_target_properties(${_EXE_TARGET} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
	)
	foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
		set_target_properties(${_EXE_TARGET} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
			LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
			RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
		)
	endforeach()

	if (_EXE_LUA_SRCS)
		check_lua_files(${_EXE_TARGET} ${_EXE_LUA_SRCS})
	endif()

	set(INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}")
	set(INSTALL_ICON_DIR "${CMAKE_INSTALL_DATADIR}/icons")
	set(INSTALL_APPLICATION_DIR "${CMAKE_INSTALL_DATADIR}/applications")

	if (_EXE_NOINSTALL)
		set(INSTALL_DATA False)
	else()
		set(INSTALL_DATA True)
	endif()

	if (APPLE)
		configure_file(${ROOT_DIR}/contrib/installer/osx/application.plist.in ${CMAKE_CURRENT_BINARY_DIR}/application.plist)
		set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/application.plist)
		set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE ON)
		set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "${CMAKE_PROJECT_NAME}.${_EXE_TARGET}")
		#set_target_properties(${_EXE_TARGET} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "YES")
		#set_target_properties(${_EXE_TARGET} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
		#set_target_properties(${_EXE_TARGET} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${CODESIGNIDENTITY})
		#set_target_properties(${_EXE_TARGET} PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${DEVELOPMENT_TEAM_ID})
		#set_target_properties(${_EXE_TARGET} PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER ${PROVISIONING_PROFILE_NAME})
		if (${CMAKE_GENERATOR} STREQUAL "Xcode")
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_EXECUTABLE_NAME \${EXECUTABLE_NAME})
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_PRODUCT_NAME \${PRODUCT_NAME})
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME \${PRODUCT_NAME})
		else()
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_PRODUCT_NAME "${_EXE_TARGET}")
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "${_EXE_TARGET}")
		endif()
		if (INSTALL_DATA)
			configure_file(${ROOT_DIR}/contrib/installer/osx/copy_dylib.sh.in ${CMAKE_CURRENT_BINARY_DIR}/copy_dylib.sh @ONLY)
			add_custom_command(TARGET ${_EXE_TARGET} POST_BUILD COMMAND cd ${CMAKE_BINARY_DIR}\; ${CMAKE_CURRENT_BINARY_DIR}/copy_dylib.sh)
		endif()
	elseif(WIN32)
		configure_file(${ROOT_DIR}/contrib/installer/windows/application.manifest.in ${CMAKE_CURRENT_BINARY_DIR}/application.manifest @ONLY)
		configure_file(${ROOT_DIR}/contrib/installer/windows/application.rc.in ${CMAKE_CURRENT_BINARY_DIR}/application.rc @ONLY)
		target_sources(${_EXE_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/application.manifest)
		target_sources(${_EXE_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/application.rc)
	elseif(UNIX)
		if (_EXE_WINDOWED)
			if (EXISTS ${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.desktop.in)
				configure_file(${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.desktop.in ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.desktop)
			else()
				configure_file(${ROOT_DIR}/contrib/installer/linux/desktop.in ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.desktop)
			endif()
			if (DESKTOP_FILE_VALIDATE_EXECUTABLE)
				add_custom_command(TARGET ${_EXE_TARGET} POST_BUILD
					COMMAND ${DESKTOP_FILE_VALIDATE_EXECUTABLE} ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.desktop
					COMMENT "Validate ${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.desktop"
				)
			endif()
			if (INSTALL_DATA)
				install(FILES ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.desktop DESTINATION ${INSTALL_APPLICATION_DIR} COMPONENT ${_EXE_TARGET})
			endif()
		endif()
		if (EXISTS ${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.service.in)
			# TODO systemd-analyze --user  verify build/Debug/src/server/vengi-server.service
			configure_file(${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.service.in ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.service)
			if (INSTALL_DATA)
				install(FILES ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.service DESTINATION lib/systemd/user COMPONENT ${_EXE_TARGET})
			endif()
		endif()
		if (EXISTS ${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.mime.in)
			configure_file(${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.mime.in ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}-mime.xml)
			if (INSTALL_DATA)
				install(FILES ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}-mime.xml DESTINATION share/mime/packages COMPONENT ${_EXE_TARGET})
			endif()
		endif()
		if (EXISTS ${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.thumbnailer.in)
			configure_file(${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.thumbnailer.in ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.thumbnailer)
			if (INSTALL_DATA)
				install(FILES ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.thumbnailer DESTINATION share/thumbnailers COMPONENT ${_EXE_TARGET})
			endif()
		endif()
		if (EXISTS ${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.man.in)
			configure_file(${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.man.in ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.6)
			if (INSTALL_DATA)
				install(FILES ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.6 DESTINATION share/man COMPONENT ${_EXE_TARGET})
			endif()
		endif()
	endif()

	set_property(GLOBAL PROPERTY ${_EXE_TARGET}_EXECUTABLE True)
	set_property(GLOBAL PROPERTY ${_EXE_TARGET}_INSTALL ${INSTALL_DATA})
	set_property(GLOBAL PROPERTY ${_EXE_TARGET}_FILES "${_EXE_FILES}")

	if (MSVC)
		set_target_properties(${_EXE_TARGET} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
	endif()

	foreach (luasrc ${_EXE_LUA_SRCS})
		get_filename_component(luasrcdir ${luasrc} DIRECTORY)
		if (INSTALL_DATA)
			install(FILES lua/${luasrc} DESTINATION ${INSTALL_DATA_DIR}/${luasrcdir} COMPONENT ${_EXE_TARGET})
		endif()
		get_filename_component(filename ${luasrc} NAME)
		get_filename_component(datafiledir ${luasrc} DIRECTORY)
		configure_file(lua/${luasrc} ${CMAKE_BINARY_DIR}/${_EXE_TARGET}/${datafiledir}/${filename} COPYONLY)
	endforeach()

	if (EXISTS ${ICON_FULL_PATH})
		set(HAS_ICON 1)
		if (INSTALL_DATA)
			install(FILES ${ICON_FULL_PATH} DESTINATION ${INSTALL_ICON_DIR} COMPONENT ${_EXE_TARGET})
		endif()
		target_sources(${_EXE_TARGET} PRIVATE ${ICON_FULL_PATH})
		set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON})
		set_source_files_properties(${ICON_FULL_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	endif()
	if (INSTALL_DATA)
		install(TARGETS ${_EXE_TARGET} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${_EXE_TARGET})
	endif()
	add_custom_target(${_EXE_TARGET}-run
		COMMAND $<TARGET_FILE:${_EXE_TARGET}>
		DEPENDS ${_EXE_TARGET}
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
	)
	engine_add_debuggger(${_EXE_TARGET})
	engine_add_valgrind(${_EXE_TARGET})
	engine_add_perf(${_EXE_TARGET})
	if (_EXE_WINDOWED)
		engina_add_vogl(${_EXE_TARGET})
	endif()
endfunction()

#
# Use this function to add binaries that are needed to build the project. E.g.
# generating code.
#
function(engine_add_build_executable)
	set(_OPTIONS_ARGS WINDOWED NOINSTALL)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS SRCS LUA_SRCS FILES)

	cmake_parse_arguments(_EXE "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	engine_add_executable("${ARGN}")
	set_property(TARGET ${_EXE_TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION False)
endfunction()

#
# Adds a new entity (src/modules) to the list of known modules. Each module can
# provide files and lua scripts to their dependent modules. This is done in a way
# that the resulting final executable will get all the files and lua scripts from
# its dependencies.
#
function(engine_add_module)
	set(_OPTIONS_ARGS)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS SRCS FILES LUA_SRCS DEPENDENCIES PRIVATE_DEPENDENCIES)

	cmake_parse_arguments(_LIB "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN})

	set(${_LIB_TARGET}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "${_LIB_TARGET} module source directory")
	generate_unity_sources(LIBRARY TARGET ${_LIB_TARGET} SRCS ${_LIB_SRCS})

	if (_LIB_LUA_SRCS)
		check_lua_files(${_LIB_TARGET} ${_LIB_LUA_SRCS})
	endif()

	set_target_properties(${_LIB_TARGET} PROPERTIES FOLDER ${_LIB_TARGET})
	if (_LIB_DEPENDENCIES)
		target_link_libraries(${_LIB_TARGET} PUBLIC ${_LIB_DEPENDENCIES})
	endif()
	if (_LIB_PRIVATE_DEPENDENCIES)
		target_link_libraries(${_LIB_TARGET} PRIVATE ${_LIB_PRIVATE_DEPENDENCIES})
	endif()

	set_property(GLOBAL PROPERTY ${_LIB_TARGET}_FILES ${_LIB_FILES})
	set_property(GLOBAL PROPERTY ${_LIB_TARGET}_LUA_SRCS ${_LIB_LUA_SRCS})
	set_property(GLOBAL PROPERTY ${_LIB_TARGET}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
	set_property(GLOBAL PROPERTY ${_LIB_TARGET}_DEPENDENCIES ${_LIB_DEPENDENCIES})
endfunction()

#
# This function will install all the target files and lua scripts. The target
# must not have the global target property <TARGET>_INSTALL disabled of course.
# E.g. the tests don't install anything, they just configure files and scripts
# into their build directory to be able to run them. But it's not needed to
# set up the whole install chain for those targets, as they are not planned to
# be delivered as an executable to any user except the developer.
#
function(engine_install_deps TARGET)
	get_property(INSTALL_DATA GLOBAL PROPERTY ${TARGET}_INSTALL)
	set(INSTALL_FILES)

	set(DEPS ${ARGV})
	list(APPEND DEPS ${TARGET})
	set(INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}-${TARGET}")

	foreach (DEP ${DEPS})
		get_property(FILES GLOBAL PROPERTY ${DEP}_FILES)
		list(APPEND INSTALL_FILES ${FILES})

		get_property(LUA_SRCS GLOBAL PROPERTY ${DEP}_LUA_SRCS)
		get_property(LUA_SRCS_DIR GLOBAL PROPERTY ${DEP}_SOURCE_DIR)

		foreach (luasrc ${LUA_SRCS})
			get_filename_component(luasrcdir ${luasrc} DIRECTORY)
			if (INSTALL_DATA)
				install(FILES ${LUA_SRCS_DIR}/lua/${luasrc} DESTINATION ${INSTALL_DATA_DIR}/${luasrcdir} COMPONENT ${TARGET})
			endif()
			get_filename_component(filename ${luasrc} NAME)
			get_filename_component(datafiledir ${luasrc} DIRECTORY)
			configure_file(${LUA_SRCS_DIR}/lua/${luasrc} ${CMAKE_BINARY_DIR}/${TARGET}/${datafiledir}/${filename} COPYONLY)
		endforeach()
	endforeach()
	if (INSTALL_FILES)
		list(REMOVE_DUPLICATES INSTALL_FILES)
		list(REVERSE INSTALL_FILES)
		foreach (datafile ${INSTALL_FILES})
			string(REGEX REPLACE "^[^/]+" "" target_datafile "${datafile}")
			string(LENGTH ${target_datafile} target_datafile_length)
			string(SUBSTRING ${target_datafile} 1 ${target_datafile_length} target_datafile)
			get_filename_component(datafiledir ${target_datafile} DIRECTORY)
			engine_install(${_LIBS_TARGET} "${datafile}" "${datafiledir}" ${INSTALL_DATA})
		endforeach()
	endif()
endfunction()

macro(engine_resolve_deps_recursive TARGET)
	get_property(DEP_DEPENDENCIES GLOBAL PROPERTY ${TARGET}_DEPENDENCIES)
	foreach (dep ${DEP_DEPENDENCIES})
		list(FIND RECURSIVE_DEPENDENCIES ${dep} already_visited)
		if (${already_visited} EQUAL -1)
			list(APPEND RECURSIVE_DEPENDENCIES ${dep})
			engine_resolve_deps_recursive(${dep})
		endif()
	endforeach()
endmacro()

#
# Perform the linking configuration. In case the given target is an executable
# we also handle the file and script installation here.
#
function(engine_target_link_libraries)
	set(_OPTIONS_ARGS)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS DEPENDENCIES)

	cmake_parse_arguments(_LIBS "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN})

	set_property(GLOBAL PROPERTY ${_LIBS_TARGET}_DEPENDENCIES ${_LIBS_DEPENDENCIES})
	target_link_libraries(${_LIBS_TARGET} ${_LIBS_DEPENDENCIES})

	get_property(EXECUTABLE GLOBAL PROPERTY ${_LIBS_TARGET}_EXECUTABLE)
	if (EXECUTABLE)
		set(RECURSIVE_DEPENDENCIES)
		engine_resolve_deps_recursive(${_LIBS_TARGET})
		engine_install_deps(${_LIBS_TARGET} ${RECURSIVE_DEPENDENCIES})
	endif()
endfunction()
