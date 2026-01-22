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

set(DOCS
	voxedit/Screenshots.md
	voxedit/Features.md
	voxedit/Configuration.md
	voxedit/Controls.md
	voxedit/usage/Console.md
	voxedit/usage/Modifiers.md
	voxedit/usage/SceneView.md
	voxedit/usage/Network.md
	voxedit/usage/Camera.md
	voxedit/usage/Viewport.md
	voxedit/usage/Assets.md
	voxedit/usage/Tools.md
	voxedit/usage/Renderer.md
	voxedit/usage/ViewModes.md
	voxedit/usage/SceneAndEditMode.md
	voxedit/usage/Memento.md
	voxedit/usage/ReferencePosition.md
	voxedit/usage/NodeInspector.md
	voxedit/usage/GameMode.md
	voxedit/usage/LSystem.md
	voxedit/usage/Animations.md
	voxedit/usage/Scripts.md
	voxedit/usage/Brushes.md
	voxedit/usage/Palette.md
	voxedit/Index.md
	voxedit/Animation.md
	Configuration.md
	FAQ.md
	Formats.md
	LUAScript.md
	Material.md
	Palette.md
	Translation.md
	img/voxedit-animation.png
	img/voxedit-asset.png
	img/voxedit-brushes.png
	img/voxedit-camera.png
	img/voxedit-console.png
	img/voxedit-cvars.png
	img/voxedit-filedialog.png
	img/voxedit-keybindings.png
	img/voxedit-lsystem.png
	img/voxedit-memento.png
	img/voxedit-nodeinspector-edit.png
	img/voxedit-nodeinspector-scene.png
	img/voxedit-options.png
	img/voxedit-palette.png
	img/voxedit-rendersettings.png
	img/voxedit-scene.png
	img/voxedit-scripts.png
	img/voxedit-timeline.png
	img/voxedit-tools-scene.png
	img/voxedit-viewport.png
)
foreach(DOC ${DOCS})
	list(APPEND FILES "../docs/${DOC}")
endforeach()

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

set(SERVER_SRCS
	VoxEditServer.h VoxEditServer.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME}server SRCS ${SERVER_SRCS} DESCRIPTION "VoxEdit server")
engine_target_link_libraries(TARGET ${PROJECT_NAME}server DEPENDENCIES app voxedit-util)
