project(shadertool)
set(SRCS
	ShaderTool.h ShaderTool.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
add_dependencies(${PROJECT_NAME} glslangValidator)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES core video simplecpp)
