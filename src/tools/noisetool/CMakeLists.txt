project(noisetool)
set(SRCS
	NoiseTool.h NoiseTool.cpp
	sauce/NoiseToolModule.h
	sauce/NoiseToolInjector.h
	ui/NoiseParametersWindow.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util)
