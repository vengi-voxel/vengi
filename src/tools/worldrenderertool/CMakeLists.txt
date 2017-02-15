project(worldrenderertool)
set(SRCS
	WorldRendererTool.h WorldRendererTool.cpp
	ui/WorldParametersWindow.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util commonlua)
check_ui_files(${PROJECT_NAME})
