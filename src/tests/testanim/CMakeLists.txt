project(testanim)
set(SRCS
	TestAnim.h TestAnim.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES testcore ozz-animation)
#generate_shaders(${PROJECT_NAME} testanim)
