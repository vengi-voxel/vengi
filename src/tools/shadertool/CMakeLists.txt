project(shadertool)
set(SRCS
	ShaderTool.h ShaderTool.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
add_dependencies(${PROJECT_NAME} glslangValidator)
target_link_libraries(${PROJECT_NAME} core video)
