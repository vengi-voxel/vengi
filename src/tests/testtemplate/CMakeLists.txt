project(testtemplate)
set(SRCS
	TestTemplate.h TestTemplate.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} frontend util)
#generate_shaders(${PROJECT_NAME} color)
