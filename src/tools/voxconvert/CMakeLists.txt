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

set(BLENDER_ADDON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/blender/io_vengi_voxconvert)
set(BLENDER_ADDON_ZIP ${CMAKE_BINARY_DIR}/io_vengi_voxconvert.zip)
add_custom_command(
	OUTPUT ${BLENDER_ADDON_ZIP}
	COMMAND ${CMAKE_COMMAND} -E tar cf ${BLENDER_ADDON_ZIP} --format=zip -- io_vengi_voxconvert
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/blender
	DEPENDS ${BLENDER_ADDON_DIR}/__init__.py
	COMMENT "Creating Blender addon zip"
)
add_custom_target(blender-addon ALL DEPENDS ${BLENDER_ADDON_ZIP})
