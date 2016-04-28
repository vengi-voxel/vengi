project(shadertool)
set(SRCS
	ShaderTool.h ShaderTool.cpp
	sauce/ShaderToolModule.h
	sauce/ShaderToolInjector.h
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
target_link_libraries(${PROJECT_NAME} core video)
