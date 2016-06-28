project(noisetool)
set(SRCS
	NoiseTool.h NoiseTool.cpp
	ui/NoiseParametersWindow.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util)
