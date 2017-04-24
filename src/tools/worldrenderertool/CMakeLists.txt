project(worldrenderertool)
set(SRCS
	WorldRendererTool.h WorldRendererTool.cpp
	ui/WorldParametersWindow.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES frontend voxel util commonlua)
check_ui_files(${PROJECT_NAME})
