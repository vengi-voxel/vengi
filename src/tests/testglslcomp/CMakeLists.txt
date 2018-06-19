project(testglslcomp)
set(SRCS
	TestGLSLComp.h TestGLSLComp.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore)
generate_shaders(${PROJECT_NAME} test)
