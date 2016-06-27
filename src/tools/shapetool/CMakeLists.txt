project(shapetool)
set(SRCS
	ShapeTool.h ShapeTool.cpp
	sauce/ShapeToolModule.h
	sauce/ShapeToolInjector.h
	ui/WorldParametersWindow.h
	ui/TreeParametersWindow.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util commonlua)
generate_shaders(${PROJECT_NAME} color)
