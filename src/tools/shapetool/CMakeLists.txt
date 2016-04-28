project(shapetool)
set(SRCS
	ShapeTool.h ShapeTool.cpp
	sauce/ShapeToolModule.h
	sauce/ShapeToolInjector.h
	ui/WorldParametersWindow.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util commonlua)
copy_data_files(${PROJECT_NAME})
