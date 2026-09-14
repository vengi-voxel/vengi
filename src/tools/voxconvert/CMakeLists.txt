project(voxconvert)
set(SRCS
	VoxConvert.h VoxConvert.cpp
)
if (EMSCRIPTEN)
	list(APPEND SRCS Emscripten.cpp)
endif()

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} DESCRIPTION "Command line voxel tool")
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES app http voxelformat voxelgenerator voxelgenerator-lua)
engine_emscripten_export_functions(${PROJECT_NAME} _get_supported_formats_json,_convert_file,_get_config_json)
if (EMSCRIPTEN)
	engine_install(${PROJECT_NAME} "${ROOT_DIR}/contrib/installer/vengi-banner-493x58.png" "" TRUE)
	engine_install(${PROJECT_NAME} "${ROOT_DIR}/tools/html/favicon.png" "" TRUE)
endif()

set(UI_SRCS
	VoxConvertUI.h VoxConvertUI.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME}ui SRCS ${UI_SRCS} WINDOWED DESCRIPTION "UI voxel tool")
engine_target_link_libraries(TARGET ${PROJECT_NAME}ui DEPENDENCIES voxelui)

enable_testing()
configure_file(tests/tests.sh.in ${PROJECT_NAME}-tests.sh @ONLY)
configure_file(tests/tests.bat.in ${PROJECT_NAME}-tests.bat @ONLY)
if (WIN32)
	add_test(NAME shelltests-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-tests.bat $<TARGET_FILE:${PROJECT_NAME}>)
else()
	add_test(NAME shelltests-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-tests.sh $<TARGET_FILE:${PROJECT_NAME}>)
endif()
set_tests_properties(shelltests-${PROJECT_NAME} PROPERTIES DEPENDS ${PROJECT_NAME})

find_program(VOXCONVERT_PYTHON_EXECUTABLE NAMES python3 python)
if (VOXCONVERT_PYTHON_EXECUTABLE)
	add_test(NAME tests-${PROJECT_NAME}-blender-addon
		COMMAND ${VOXCONVERT_PYTHON_EXECUTABLE} -m unittest discover -s "${CMAKE_CURRENT_SOURCE_DIR}/blender/tests" -v
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blender/tests
	)
endif()

find_program(BLENDER_EXECUTABLE NAMES blender blender.exe)
if (BLENDER_EXECUTABLE)
	add_test(NAME tests-${PROJECT_NAME}-blender-smoke
		COMMAND ${BLENDER_EXECUTABLE} --background --factory-startup --python-exit-code 1
			--python "${CMAKE_CURRENT_SOURCE_DIR}/blender/tests/blender_smoke.py"
			-- $<TARGET_FILE:${PROJECT_NAME}>
	)
	set_tests_properties(tests-${PROJECT_NAME}-blender-smoke PROPERTIES DEPENDS ${PROJECT_NAME})
endif()

set(BLENDER_ADDON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/blender/io_vengi_voxconvert)
set(BLENDER_ADDON_STAGE ${CMAKE_CURRENT_BINARY_DIR}/io_vengi_voxconvert)
set(BLENDER_ADDON_ZIP ${CMAKE_BINARY_DIR}/io_vengi_voxconvert.zip)
add_custom_command(
	OUTPUT ${BLENDER_ADDON_ZIP}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${BLENDER_ADDON_STAGE}
	COMMAND ${CMAKE_COMMAND} -E copy
		${BLENDER_ADDON_DIR}/__init__.py
		${BLENDER_ADDON_DIR}/util.py
		${BLENDER_ADDON_DIR}/blender_manifest.toml
		${BLENDER_ADDON_STAGE}
	COMMAND ${CMAKE_COMMAND} -E tar cf ${BLENDER_ADDON_ZIP} --format=zip -- io_vengi_voxconvert
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS
		${BLENDER_ADDON_DIR}/__init__.py
		${BLENDER_ADDON_DIR}/util.py
		${BLENDER_ADDON_DIR}/blender_manifest.toml
	COMMENT "Creating Blender addon zip"
)
add_custom_target(blender-addon ALL DEPENDS ${BLENDER_ADDON_ZIP})

if (NOT EMSCRIPTEN)
	if (WIN32)
		set(BLENDER_ADDON_INSTALL_DIR "${PROJECT_NAME}")
	elseif (APPLE)
		set(BLENDER_ADDON_INSTALL_DIR ".")
	else()
		set(BLENDER_ADDON_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/${CMAKE_PROJECT_NAME}-${PROJECT_NAME}")
	endif()
	install(FILES ${BLENDER_ADDON_ZIP} DESTINATION ${BLENDER_ADDON_INSTALL_DIR} COMPONENT voxconvertblender)
	cpack_add_component(voxconvertblender DISPLAY_NAME "Blender addon" DESCRIPTION "Blender import/export addon for vengi-voxconvert")
endif()
