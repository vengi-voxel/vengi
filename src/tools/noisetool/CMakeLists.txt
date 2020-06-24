project(noisetool)
set(SRCS
	NoiseTool.h NoiseTool.cpp
	NoiseData.h
	NoiseType.h NoiseType.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES imgui frontend)
