project(shapetool)
set(SRCS
	ShapeTool.h ShapeTool.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES ui frontend util commonlua)
check_ui_files(${PROJECT_NAME})
