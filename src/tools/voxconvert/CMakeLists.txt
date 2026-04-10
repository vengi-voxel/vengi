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

engine_generate_standalone_wix(
	TARGET ${PROJECT_NAME}
	TEMPLATE ${ROOT_DIR}/contrib/installer/windows/voxconvert.wxs.in
	OUTPUT ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.wxs
	PRODUCT_NAME "Vengi Voxconvert"
	UPGRADE_GUID "3E8B7A21-5D4C-4F6E-A9B8-1C2D3E4F5A67"
	UI_TARGET ${PROJECT_NAME}ui
)

enable_testing()
configure_file(tests/tests.sh.in ${PROJECT_NAME}-tests.sh @ONLY)
configure_file(tests/tests.bat.in ${PROJECT_NAME}-tests.bat @ONLY)
if (WIN32)
	add_test(NAME shelltests-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-tests.bat $<TARGET_FILE:${PROJECT_NAME}>)
else()
	add_test(NAME shelltests-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-tests.sh $<TARGET_FILE:${PROJECT_NAME}>)
endif()
set_tests_properties(shelltests-${PROJECT_NAME} PROPERTIES DEPENDS ${PROJECT_NAME})
