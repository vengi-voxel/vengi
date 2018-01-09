project(worldrenderertool)
set(SRCS
	WorldRendererTool.h WorldRendererTool.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES imgui frontend voxel util commonlua)
check_ui_turbobadger(${PROJECT_NAME})
