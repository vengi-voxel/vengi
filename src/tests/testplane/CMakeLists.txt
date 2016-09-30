project(testplane)
set(SRCS
	TestPlane.h TestPlane.cpp
)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
target_link_libraries(${PROJECT_NAME} testcore)
