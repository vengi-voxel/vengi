function(engine_install TARGET FILE DESTINATION INSTALL_DATA)
	if (WIN32)
		set(INSTALL_DATA_DIR ".")
	else()
		set(INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}-${TARGET}")
	endif()

	get_filename_component(filename ${FILE} NAME)
	target_sources(${TARGET} PRIVATE ${DATA_DIR}/${FILE})
	if (APPLE)
		set_source_files_properties(${DATA_DIR}/${FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${DESTINATION})
	elseif (INSTALL_DATA)
		install(FILES ${DATA_DIR}/${FILE} DESTINATION ${INSTALL_DATA_DIR}/${DESTINATION} COMPONENT ${TARGET})
	endif()
	configure_file(${DATA_DIR}/${FILE} ${CMAKE_BINARY_DIR}/${TARGET}/${DESTINATION}/${filename} COPYONLY)
endfunction()

function(engine_compressed_file_to_header NAME INPUT_FILE OUTPUT_FILE)
	if (NOT CMAKE_CROSSCOMPILING)
		add_custom_command(
			OUTPUT ${OUTPUT_FILE}
			COMMAND
				binary_to_compressed_c
				${INPUT_FILE}
				${NAME} > ${OUTPUT_FILE}
			DEPENDS ${INPUT_FILE} binary_to_compressed_c
			COMMENT "Generate c header for compressed ${INPUT_FILE} in ${OUTPUT_FILE}"
			VERBATIM
		)
		add_custom_target(engine_compressed_file_to_header_${NAME} DEPENDS ${OUTPUT_FILE})
		add_dependencies(codegen engine_compressed_file_to_header_${NAME})
	elseif (NOT EXISTS ${OUTPUT_FILE})
		message(STATUS "Source code generation must be done by native toolchain")
	endif()
	engine_mark_as_generated(${OUTPUT_FILE})
endfunction()

function(engine_file_to_header NAME INPUT_FILE OUTPUT_FILE)
	if (NOT CMAKE_CROSSCOMPILING)
		add_custom_command(
			OUTPUT ${OUTPUT_FILE}
			COMMAND
				binary_to_compressed_c
				-nocompress
				${INPUT_FILE}
				${NAME} > ${OUTPUT_FILE}
			DEPENDS ${INPUT_FILE} binary_to_compressed_c
			COMMENT "Generate c header for ${INPUT_FILE} in ${OUTPUT_FILE}"
			VERBATIM
		)
		add_custom_target(engine_file_to_header_${NAME} DEPENDS ${OUTPUT_FILE})
		add_dependencies(codegen engine_file_to_header_${NAME})
	elseif (NOT EXISTS ${OUTPUT_FILE})
		message(STATUS "Source code generation must be done by native toolchain")
	endif()
	engine_mark_as_generated(${OUTPUT_FILE})
endfunction()

function(engine_add_sharedlibrary)
	set(_OPTIONS_ARGS)
	set(_ONE_VALUE_ARGS TARGET)
	set(_MULTI_VALUE_ARGS SRCS)

	cmake_parse_arguments(_LIB "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	add_library(${_LIB_TARGET} SHARED ${_LIB_SRCS})

	set_target_properties(${_LIB_TARGET} PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}-${_LIB_TARGET}")
	set_target_properties(${_LIB_TARGET} PROPERTIES
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_LIB_TARGET}"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_LIB_TARGET}"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${_LIB_TARGET}"
	)
	foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
		set_target_properties(${_LIB_TARGET} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${_LIB_TARGET}"
			LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${_LIB_TARGET}"
			RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/${_LIB_TARGET}"
		)
	endforeach()
	if (WIN32)
		set(INSTALL_LIB_DIR ".")
	else()
		set(INSTALL_LIB_DIR ".")
	endif()
	install(TARGETS ${_LIB_TARGET} DESTINATION ${INSTALL_LIB_DIR} COMPONENT ${_LIB_TARGET})
	set(CPACK_NSIS_${_LIB_TARGET}_INSTALL_DIRECTORY ${_LIB_TARGET})
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
	set(_ONE_VALUE_ARGS TARGET DESCRIPTION)
	set(_MULTI_VALUE_ARGS SRCS LUA_SRCS FILES)

	cmake_parse_arguments(_EXE "${_OPTIONS_ARGS}" "${_ONE_VALUE_ARGS}" "${_MULTI_VALUE_ARGS}" ${ARGN} )

	# e.g. used in desktop files
	set(COMMANDLINE "${CMAKE_PROJECT_NAME}-${_EXE_TARGET}")
	set(CATEGORIES "Game")
	set(DESCRIPTION "${_EXE_DESCRIPTION}")
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

	set(CPACK_NSIS_${_EXE_TARGET}_INSTALL_DIRECTORY ${_EXE_TARGET})

	set(${_EXE_TARGET}_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "${_EXE_TARGET} source directory")
	if (_EXE_WINDOWED)
		if (WINDOWS)
			add_executable(${_EXE_TARGET} WIN32 ${_EXE_SRCS})
			if (USE_MSVC)
				set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
			endif()
		elseif(APPLE)
			add_executable(${_EXE_TARGET} MACOSX_BUNDLE ${_EXE_SRCS})
		else()
			add_executable(${_EXE_TARGET} ${_EXE_SRCS})
		endif()
	else()
		add_executable(${_EXE_TARGET} ${_EXE_SRCS})
		if (WINDOWS)
			if (USE_MSVC)
				set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
			endif()
		endif()
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

	if (WIN32)
		set(INSTALL_DATA_DIR ".")
		set(INSTALL_ICON_DIR ".")
		set(INSTALL_APPLICATION_DIR "unused")
		set(INSTALL_METAINFO_DIR "unused")
		set(INSTALL_BIN_DIR ".")
	elseif(APPLE)
		set(INSTALL_DATA_DIR ".")
		set(INSTALL_ICON_DIR "unused")
		set(INSTALL_APPLICATION_DIR "unused")
		set(INSTALL_METAINFO_DIR "unused")
		set(INSTALL_BIN_DIR ".")
	else()
		set(INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}")
		set(INSTALL_ICON_DIR "${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps")
		set(INSTALL_APPLICATION_DIR "${CMAKE_INSTALL_DATADIR}/applications")
		set(INSTALL_METAINFO_DIR "${CMAKE_INSTALL_DATADIR}/metainfo")
		set(INSTALL_BIN_DIR "${CMAKE_INSTALL_BINDIR}")
	endif()

	if (_EXE_NOINSTALL)
		set(INSTALL_DATA False)
		set(CPACK_${_EXE_TARGET}_COMPONENT_INSTALL OFF)
	else()
		set(INSTALL_DATA True)
		set(CPACK_${_EXE_TARGET}_COMPONENT_INSTALL ON)
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
		set(TILE_BIG_FULL_PATH ${DATA_DIR}/${_EXE_TARGET}/${_EXE_TARGET}-tile.png)
		set(TILE_SMALL_FULL_PATH ${DATA_DIR}/${_EXE_TARGET}/${_EXE_TARGET}-tile-small.png)
		if (EXISTS ${TILE_BIG_FULL_PATH} AND EXISTS ${TILE_SMALL_FULL_PATH})
			install(FILES ${TILE_BIG_FULL_PATH} DESTINATION ${INSTALL_ICON_DIR} COMPONENT ${_EXE_TARGET})
			install(FILES ${TILE_SMALL_FULL_PATH} DESTINATION ${INSTALL_ICON_DIR} COMPONENT ${_EXE_TARGET})
			configure_file(${ROOT_DIR}/contrib/installer/windows/application.VisualElementsManifest.xml.in ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.VisualElementsManifest.xml @ONLY)
		endif()
		configure_file(${ROOT_DIR}/contrib/installer/windows/application.manifest.in ${CMAKE_CURRENT_BINARY_DIR}/application.manifest @ONLY)
		configure_file(${ROOT_DIR}/contrib/installer/windows/application.rc.in ${CMAKE_CURRENT_BINARY_DIR}/application.rc @ONLY)
		target_sources(${_EXE_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/application.manifest)
		target_sources(${_EXE_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/application.rc)
		# if (NOT _EXE_NOINSTALL AND _EXE_DESCRIPTION)
		# 	list(APPEND CPACK_PACKAGE_EXECUTABLES "${_EXE_TARGET};${_EXE_DESCRIPTION}")
		# endif()
	elseif (EMSCRIPTEN)
		if (_EXE_WINDOWED)
			#em_link_js_library(${_EXE_TARGET} ${ROOT_DIR}/contrib/installer/emscripten/library.js)
			configure_file(${ROOT_DIR}/contrib/installer/emscripten/index.html.in ${CMAKE_CURRENT_BINARY_DIR}/index.html @ONLY)
			set_target_properties(${_EXE_TARGET} PROPERTIES SUFFIX ".html")
			set_target_properties(${_EXE_TARGET} PROPERTIES LINK_FLAGS "--shell-file ${CMAKE_CURRENT_BINARY_DIR}/index.html")
			configure_file(${ROOT_DIR}/contrib/installer/emscripten/coi-serviceworker.js ${CMAKE_BINARY_DIR}/${_EXE_TARGET}/coi-serviceworker.js COPYONLY)
		endif()
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
				install(FILES ${ROOT_DIR}/contrib/installer/linux/io.github.mgerhardy.vengi.voxedit.metainfo.xml DESTINATION ${INSTALL_METAINFO_DIR} COMPONENT ${_EXE_TARGET})
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
			set(MAN_PAGE_SECTION 1)
			file(STRINGS ${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.man.in MAN_PAGE_HEADER REGEX "^\.TH")
			string(REPLACE " " ";" MAN_PAGE_HEADER ${MAN_PAGE_HEADER})
			list(LENGTH MAN_PAGE_HEADER MAN_PAGE_HEADER_LEN)
			if (MAN_PAGE_HEADER_LEN GREATER_EQUAL 3)
				list(GET MAN_PAGE_HEADER 2 MAN_PAGE_SECTION)
				string(REPLACE "\"" "" MAN_PAGE_SECTION ${MAN_PAGE_SECTION})
			endif()
			configure_file(${ROOT_DIR}/contrib/installer/linux/${_EXE_TARGET}.man.in ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.${MAN_PAGE_SECTION})
			if (INSTALL_DATA)
				install(FILES ${PROJECT_BINARY_DIR}/${CMAKE_PROJECT_NAME}-${_EXE_TARGET}.${MAN_PAGE_SECTION} DESTINATION share/man/man${MAN_PAGE_SECTION} COMPONENT ${_EXE_TARGET})
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
		if (APPLE)
			set_source_files_properties(lua/${luasrc} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${luasrcdir})
		elseif (INSTALL_DATA)
			install(FILES lua/${luasrc} DESTINATION ${INSTALL_DATA_DIR}/${luasrcdir} COMPONENT ${_EXE_TARGET})
		endif()
		get_filename_component(filename ${luasrc} NAME)
		get_filename_component(datafiledir ${luasrc} DIRECTORY)
		configure_file(lua/${luasrc} ${CMAKE_BINARY_DIR}/${_EXE_TARGET}/${datafiledir}/${filename} COPYONLY)
	endforeach()

	if (HAS_ICON)
		if (APPLE)
			set_target_properties(${_EXE_TARGET} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON})
			set_source_files_properties(${ICON_FULL_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
		elseif (INSTALL_DATA)
			install(FILES ${ICON_FULL_PATH} DESTINATION ${INSTALL_ICON_DIR} COMPONENT ${_EXE_TARGET})
		endif()
		target_sources(${_EXE_TARGET} PRIVATE ${ICON_FULL_PATH})
	endif()
	if (INSTALL_DATA)
		install(TARGETS ${_EXE_TARGET} DESTINATION ${INSTALL_BIN_DIR} COMPONENT ${_EXE_TARGET})
	endif()
	add_custom_target(${_EXE_TARGET}-run
		COMMAND $<TARGET_FILE:${_EXE_TARGET}>
		USES_TERMINAL
		DEPENDS ${_EXE_TARGET}
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/${_EXE_TARGET}"
	)
	engine_add_debuggger(${_EXE_TARGET})
	engine_add_valgrind(${_EXE_TARGET})
	engine_add_perf(${_EXE_TARGET})
	engine_add_heaptrack(${_EXE_TARGET})
	engine_add_apitrace(${_EXE_TARGET})
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
	add_library(${_LIB_TARGET} ${_LIB_SRCS})

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

	if (USE_CPPCHECK)
		find_program(CPPCHECK cppcheck)
		if (CPPCHECK)
			set(cppcheck_cmd
				${CPPCHECK}
				"--enable=warning,performance,portability"
				"--inconclusive"
				"--force"
				"--quiet"
				"--inline-suppr"
				"--std=c++11"
				"--suppressions-list=${SCRIPTS_CMAKE_DIR}/CppCheckSuppressions.txt"
			)
			set_target_properties(${_LIB_TARGET} PROPERTIES CXX_CPPCHECK "${cppcheck_cmd}")
		endif()
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

	set(DEPS ${ARGN})
	list(APPEND DEPS ${TARGET})
	if (WIN32)
		set(INSTALL_DATA_DIR ".")
	else()
		set(INSTALL_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}-${TARGET}")
	endif()

	foreach (DEP ${DEPS})
		get_property(FILES GLOBAL PROPERTY ${DEP}_FILES)
		list(APPEND INSTALL_FILES ${FILES})

		get_property(LUA_SRCS GLOBAL PROPERTY ${DEP}_LUA_SRCS)
		get_property(LUA_SRCS_DIR GLOBAL PROPERTY ${DEP}_SOURCE_DIR)

		foreach (luasrc ${LUA_SRCS})
			get_filename_component(luasrcdir ${luasrc} DIRECTORY)
			if (APPLE)
				set_source_files_properties(lua/${luasrc} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${luasrcdir})
			elseif (INSTALL_DATA)
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

macro(engine_resolve_deps_recursive TARGET OUT)
	set(RECURSIVE_DEPENDENCIES ${${OUT}})
	get_property(DEP_DEPENDENCIES GLOBAL PROPERTY ${TARGET}_DEPENDENCIES)
	foreach (dep ${DEP_DEPENDENCIES})
		list(FIND RECURSIVE_DEPENDENCIES ${dep} already_visited)
		if (${already_visited} EQUAL -1)
			list(APPEND RECURSIVE_DEPENDENCIES ${dep})
			engine_resolve_deps_recursive(${dep} RECURSIVE_DEPENDENCIES)
		endif()
	endforeach()
	set(${OUT} ${RECURSIVE_DEPENDENCIES})
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
		engine_resolve_deps_recursive(${_LIBS_TARGET} RECURSIVE_DEPENDENCIES)
		engine_install_deps(${_LIBS_TARGET} ${RECURSIVE_DEPENDENCIES})
	endif()
endfunction()
