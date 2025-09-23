include_directories(modules)
add_subdirectory(modules)

project(voxedit)
set(SRCS
	VoxEdit.h VoxEdit.cpp
	VoxEditMain.cpp
)

set(FILES
	voxedit/scripts/test-layers.cfg
	voxedit/scripts/test-animate.cfg
	shared/font.ttf
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED DESCRIPTION "Voxel Editor")
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES voxedit-ui voxedit-util voxelgenerator-lua)
engine_emscripten_export_functions(${PROJECT_NAME})
if (EMSCRIPTEN)
	engine_install(${PROJECT_NAME} "${ROOT_DIR}/tools/html/favicon.png" "" TRUE)
endif()

# headless ui tests
if (USE_IMGUITESTENGINE)
	set(TEST_SRCS
		VoxEdit.h VoxEdit.cpp
		VoxEditTest.cpp
	)
	engine_add_executable(TARGET tests-voxedit SRCS ${TEST_SRCS} FILES ${FILES} NOINSTALL DESCRIPTION "Voxel Editor Tests")
	engine_target_link_libraries(TARGET tests-voxedit DEPENDENCIES voxedit-ui voxedit-util)
	add_test(NAME tests-voxedit COMMAND $<TARGET_FILE:tests-voxedit> WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()
